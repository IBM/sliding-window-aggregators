use std::cell::RefCell;

use alga::general::AbstractMagma;
use alga::general::Identity;

use crate::FifoWindow;
use crate::ops::AggregateOperator;
use crate::ops::AggregateMonoid;

const LOW_CAP: usize = 2;

#[derive(Clone)]
pub struct FlatFIT<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone,
{
    front: usize,
    back: usize,
    size: usize,
    buffer: RefCell<Vec<Item<BinOp::Partial>>>,
    tracing_indices: RefCell<Vec<usize>>,
}

#[derive(Clone)]
struct Item<Value> {
    val: Value,
    next: usize,
}

impl<Value> Item<Value> {
    fn new(val: Value, next: usize) -> Self {
        Self { val, next }
    }
}

impl<BinOp> FifoWindow<BinOp> for FlatFIT<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone,
{
    fn new() -> Self {
        Self {
            front: 0,
            back: 0,
            size: 0,
            buffer: RefCell::new(Vec::new()),
            tracing_indices: RefCell::new(Vec::new()),
        }
    }
    fn name() -> &'static str {
        "flatfit"
    }
    fn push(&mut self, val: BinOp::In) {
        let capacity = self.buffer.borrow().capacity();
        if self.size + 1 > capacity {
            self.rescale(capacity * 2)
        }
        let mut buffer = self.buffer.borrow_mut();
        self.size += 1;
        let prev = self.back;
        self.back = (self.back + 1) % self.size;
        buffer[self.back].val = BinOp::lift(val);
        buffer[prev].next = self.back;
    }
    fn pop(&mut self) {
        if self.size > 0 {
            let capacity = self.buffer.borrow().capacity();
            self.front = (self.front + 1) % capacity;
            self.size -= 1;
            if self.size < capacity / 2 {
                self.rescale(capacity / 2)
            }
        }
    }
    fn query(&self) -> BinOp::Out {
        let mut agg = BinOp::Partial::identity();
        let mut buffer = self.buffer.borrow_mut();
        if self.size > 0 {
            let mut tracing_indices = self.tracing_indices.borrow_mut();
            debug_assert!(tracing_indices.is_empty());
            let mut current = self.front;
            while current != self.back {
                tracing_indices.push(current);
                current = buffer[current].next;
            }
            // FlatFIT queries mutate the internal buffer
            while let Some(i) = tracing_indices.pop() {
                agg = agg.operate(&buffer[i].val);
                buffer[i] = Item::new(agg.clone(), self.back);
            }
            agg = agg.operate(&buffer[self.back].val);
        }
        BinOp::lower(&agg)
    }
    fn len(&self) -> usize {
        self.size
    }
    fn is_empty(&self) -> bool {
        self.size == 0
    }
}

impl<BinOp> FlatFIT<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone,
{
    fn rescale(&mut self, new_capacity: usize) {
        let new_capacity = std::cmp::max(new_capacity, LOW_CAP);
        let mut new_buffer: Vec<Item<BinOp::Partial>> = Vec::with_capacity(new_capacity);
        unsafe {
            // Unsafe is used here so we don't need to initialize the buffer's contents
            new_buffer.set_len(new_capacity);
        }
        let mut buffer = self.buffer.borrow_mut();
        let old_capacity = buffer.len();
        for i in 0..self.size {
            let item = &buffer[(self.front + i) % old_capacity];
            let offset = (item.next + old_capacity - self.front) % old_capacity;
            new_buffer[i].val = item.val.clone();
            new_buffer[i].next = offset;
        }
        *buffer = new_buffer;
        self.front = 0;
        if self.size == 0 {
            self.back = 0;
        } else {
            self.back = self.size - 1
        }
    }
}
