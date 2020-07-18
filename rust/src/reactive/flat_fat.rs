use alga::general::AbstractMonoid;
use alga::general::Operator;
use std::collections::HashSet;
use std::marker::PhantomData;

pub(crate) trait FAT<Value, BinOp>: Clone
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    /// Creates a new FAT from a batch of values
    fn new(batch: &[Value]) -> Self;

    /// Creates a new FAT with uninitialized values
    fn with_capacity(capacity: usize) -> Self;

    /// Updates a non-contiguous batch of leaves
    fn update<I>(&mut self, batch: I)
    where
        I: IntoIterator<Item = (usize, Value)>;

    /// Updates a contiguous batch of leaves
    fn update_ordered<I>(&mut self, values: I)
    where
        I: IntoIterator<Item = Value>;

    /// Aggregates all nodes in the FAT and returns the result
    fn aggregate(&self) -> Value;

    /// Aggregates a prefix of nodes in the FAT and returns the result
    fn prefix(&self, i: usize) -> Value;

    /// Aggregates a suffix of nodes in the FAT and returns the result
    fn suffix(&self, i: usize) -> Value;
}

#[derive(Clone)]
pub(crate) struct FlatFAT<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    /// A flat binary tree, indexed as:
    ///       0
    ///      / \
    ///     /   \
    ///    1     2
    ///   / \   / \
    ///  3   4 5   6
    pub(crate) tree: Vec<Value>,
    /// Number of leaves which can be stored in the tree
    pub(crate) capacity: usize,
    binop: PhantomData<BinOp>,
}

impl<Value, BinOp> FlatFAT<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    /// Returns all leaf nodes of the tree
    pub(crate) fn leaves(&self) -> &[Value] {
        &self.tree[self.leaf(0)..]
    }
    /// Returns the index of the root node
    fn root(&self) -> usize {
        0
    }
    /// Returns the index of a leaf node
    fn leaf(&self, i: usize) -> usize {
        i + self.capacity - 1
    }
    /// Returns the index of an node's left child
    fn left(&self, i: usize) -> usize {
        2 * (i + 1) - 1
    }
    /// Returns the index of an node's right child
    fn right(&self, i: usize) -> usize {
        2 * (i + 1)
    }
    /// Returns the index of an node's parent
    fn parent(&self, i: usize) -> usize {
        (i - 1) / 2
    }
}

impl<Value, BinOp> FAT<Value, BinOp> for FlatFAT<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn new(values: &[Value]) -> Self {
        let capacity = values.len();
        let mut new = Self::with_capacity(capacity);
        new.update_ordered(values.iter().cloned());
        new
    }

    fn with_capacity(capacity: usize) -> Self {
        assert_ne!(capacity, 0, "Capacity of window must be greater than 0");
        Self {
            tree: vec![Value::identity(); 2 * capacity - 1],
            binop: PhantomData,
            capacity,
        }
    }

    fn update<I>(&mut self, batch: I)
    where
        I: IntoIterator<Item = (usize, Value)>,
    {
        let mut parents: HashSet<usize> = batch
            .into_iter()
            .map(|(idx, val)| {
                let leaf = self.leaf(idx);
                self.tree[leaf] = val;
                self.parent(leaf)
            })
            .collect();
        let mut new_parents: HashSet<usize> = HashSet::new();
        loop {
            parents.drain().for_each(|parent| {
                let left = self.left(parent);
                let right = self.right(parent);
                self.tree[parent] = self.tree[left].operate(&self.tree[right]);
                if parent != self.root() {
                    new_parents.insert(self.parent(parent));
                }
            });
            if new_parents.is_empty() {
                break;
            } else {
                std::mem::swap(&mut parents, &mut new_parents);
            }
        }
    }

    fn update_ordered<I>(&mut self, values: I)
    where
        I: IntoIterator<Item = Value>,
    {
        values.into_iter().enumerate().for_each(|(idx, val)| {
            let leaf = self.leaf(idx);
            self.tree[leaf] = val;
        });
        (0..self.leaf(0)).rev().for_each(|parent| {
            let left = self.left(parent);
            let right = self.right(parent);
            self.tree[parent] = self.tree[left].operate(&self.tree[right]);
        });
    }

    fn aggregate(&self) -> Value {
        self.tree[self.root()].clone()
    }

    fn prefix(&self, idx: usize) -> Value {
        let mut node = self.leaf(idx);
        let mut agg = self.tree[node].clone();
        while node != self.root() {
            let parent = self.parent(node);
            if node == self.right(parent) {
                let left = self.left(parent);
                agg = self.tree[left].operate(&agg);
            }
            node = parent;
        }
        agg
    }

    fn suffix(&self, i: usize) -> Value {
        let mut node = self.leaf(i);
        let mut agg = self.tree[node].clone();
        while node != self.root() {
            let parent = self.parent(node);
            if node == self.left(parent) {
                agg = agg.operate(&self.tree[self.right(parent)]);
            }
            node = parent;
        }
        agg
    }
}
