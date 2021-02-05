use alga::general::AbstractMagma;
use alga::general::Identity;

use fxhash::FxHashSet as HashSet;

use crate::ops::AggregateOperator;
use crate::ops::AggregateMonoid;

pub(crate) trait FAT<BinOp>: Clone
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    /// Creates a new FAT from a batch of values
    fn new(batch: &[BinOp::Partial]) -> Self;

    /// Creates a new FAT with uninitialized values
    fn with_capacity(capacity: usize) -> Self;

    /// Returns the value of the leaf at `idx`
    fn get(&self, idx: usize) -> Option<&BinOp::Partial>;

    /// Updates a non-contiguous batch of leaves
    fn update<I>(&mut self, batch: I)
    where
        I: IntoIterator<Item = (usize, BinOp::Partial)>;

    /// Updates a contiguous batch of leaves
    fn update_ordered<I>(&mut self, values: I)
    where
        I: IntoIterator<Item = BinOp::Partial>;

    /// Aggregates all nodes in the FAT and returns the result
    fn aggregate(&self) -> BinOp::Partial;

    /// Aggregates a prefix of nodes in the FAT and returns the result
    fn prefix(&self, i: usize) -> BinOp::Partial;

    /// Aggregates a suffix of nodes in the FAT and returns the result
    fn suffix(&self, i: usize) -> BinOp::Partial;
}

#[derive(Clone)]
pub(crate) struct FlatFAT<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    /// A flat binary tree, indexed as:
    ///       0
    ///      / \
    ///     /   \
    ///    1     2
    ///   / \   / \
    ///  3   4 5   6
    pub(crate) tree: Vec<BinOp::Partial>,
    /// Number of leaves which can be stored in the tree
    pub(crate) capacity: usize,
}

impl<BinOp> FlatFAT<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    /// Returns all leaf nodes of the tree
    #[inline(always)]
    pub(crate) fn leaves(&self) -> &[BinOp::Partial] {
        &self.tree[self.leaf(0)..]
    }
    /// Returns the index of the root node
    #[inline(always)]
    fn root(&self) -> usize {
        0
    }
    /// Returns the index of a leaf node
    #[inline(always)]
    fn leaf(&self, i: usize) -> usize {
        i + self.capacity - 1
    }
    /// Returns the index of an node's left child
    #[inline(always)]
    fn left(&self, i: usize) -> usize {
        2 * i + 1
    }
    /// Returns the index of an node's right child
    #[inline(always)]
    fn right(&self, i: usize) -> usize {
        2 * i + 2
    }
    /// Returns the index of an node's parent
    #[inline(always)]
    fn parent(&self, i: usize) -> usize {
        (i - 1) / 2
    }
}

impl<BinOp> FAT<BinOp> for FlatFAT<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    fn new(values: &[BinOp::Partial]) -> Self {
        let capacity = values.len();
        let mut new = Self::with_capacity(capacity);
        new.update_ordered(values.iter().cloned());
        new
    }

    fn with_capacity(capacity: usize) -> Self {
        assert!(capacity > 1, "Capacity of window must be greater than 1");
        Self {
            tree: vec![BinOp::Partial::identity(); 2 * capacity - 1],
            capacity,
        }
    }

    fn get(&self, idx: usize) -> Option<&BinOp::Partial> {
        self.tree.get(self.leaf(idx))
    }

    fn update<I>(&mut self, batch: I)
    where
        I: IntoIterator<Item = (usize, BinOp::Partial)>,
    {
        let mut parents: HashSet<usize> = batch
            .into_iter()
            .map(|(idx, val)| {
                let leaf = self.leaf(idx);
                self.tree[leaf] = val;
                self.parent(leaf)
            })
            .collect();
        let mut new_parents: HashSet<usize> = HashSet::default();
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
        I: IntoIterator<Item = BinOp::Partial>,
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

    fn aggregate(&self) -> BinOp::Partial {
        self.tree[self.root()].clone()
    }

    fn prefix(&self, idx: usize) -> BinOp::Partial {
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

    fn suffix(&self, i: usize) -> BinOp::Partial {
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
