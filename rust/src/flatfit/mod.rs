use crate::FifoWindow;
use alga::general::AbstractMonoid;
use alga::general::Operator;
use std::marker::PhantomData;

const LOW_CAP: usize = 2;

#[derive(Clone)]
pub struct FlatFIT<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    front: usize,
    back: usize,
    size: usize,
    buffer: Vec<Item<Value>>,
    binop: PhantomData<BinOp>,
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

impl<Value, BinOp> FifoWindow<Value, BinOp> for FlatFIT<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn new() -> Self {
        Self {
            front: 0,
            back: 0,
            size: 0,
            buffer: Vec::new(),
            binop: PhantomData,
        }
    }
    fn push(&mut self, val: Value) {
        if self.size + 1 > self.buffer.len() {
            self.rescale(self.buffer.len() * 2)
        }
        self.size += 1;
        let prev = self.back;
        self.back = (self.back + 1) % self.size;
        self.buffer[self.back].val = val;
        self.buffer[prev].next = self.back;
    }
    fn pop(&mut self) -> Option<Value> {
        if self.size > 0 {
            let item = self.buffer.get(self.front).map(|item| item.val.clone());
            self.front = (self.front + 1) % self.buffer.capacity();
            self.size -= 1;
            if self.size < self.buffer.len() / 2 {
                self.rescale(self.buffer.len() / 2)
            }
            item
        } else {
            None
        }
    }
    fn query(&self) -> Value {
        let mut agg = Value::identity();
        if self.size > 0 {
            let mut tracing_indices = Vec::new();
            let mut current = self.front;
            while current != self.back {
                tracing_indices.push(current);
                current = self.buffer[current].next;
            }
            unsafe {
                // FlatFIT queries mutate the internal buffer
                let buffer = &self.buffer as *const Vec<Item<Value>> as *mut Vec<Item<Value>>;
                while let Some(i) = tracing_indices.pop() {
                    agg = agg.operate(&self.buffer[i].val);
                    (*buffer)[i] = Item::new(agg.clone(), self.back);
                }
            }
            agg = agg.operate(&self.buffer[self.back].val);
        }
        agg
    }
    fn len(&self) -> usize {
        self.size
    }
    fn is_empty(&self) -> bool {
        self.size == 0
    }
}

impl<Value, BinOp> FlatFIT<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn rescale(&mut self, new_capacity: usize) {
        let new_capacity = std::cmp::max(new_capacity, LOW_CAP);
        let mut new_buffer: Vec<Item<Value>> = Vec::with_capacity(new_capacity);
        unsafe {
            // Unsafe is used here so we don't need to initialize the buffer's contents
            new_buffer.set_len(new_capacity);
        }
        let old_capacity = self.buffer.capacity();
        for i in 0..self.size {
            let item = &self.buffer[(self.front + i) % old_capacity];
            let offset = (item.next + old_capacity - self.front) % old_capacity;
            new_buffer[i].val = item.val.clone();
            new_buffer[i].next = offset;
        }
        self.buffer = new_buffer;
        self.front = 0;
        if self.size == 0 {
            self.back = 0;
        } else {
            self.back = self.size - 1
        }
    }
}
