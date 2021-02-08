use alga::general::AbstractGroup;
use alga::general::AbstractMonoid;
use alga::general::Identity;
use alga::general::Operator;

/// Abstract Algebra Lattice:
/// Borrowed from https://docs.rs/alga/0.9.3/alga/general/index.html
///
/// ```text
///                           AbstractMagma
///                                |
///                        _______/ \______
///                       /                \
///                 divisibility       associativity
///                      |                  |
///                      V                  V
///                AbstractQuasigroup AbstractSemigroup
///                      |                  |
///                  identity            identity
///                      |                  |
///                      V                  V
///                 AbstractLoop       AbstractMonoid
///                      |                  |
///                 associativity     invertibility
///                       \______   _______/
///                              \ /
///                               |
///                               V
///                         AbstractGroup
///                               |
///                         commutativity
///                               |
///                               V
///                     AbstractGroupAbelian
/// ```

pub trait AggregateOperator: Operator {
    type In: Clone;
    type Out: Clone;
}

pub trait AggregateMonoid<BinOp: AggregateOperator>  {
    type Partial: Identity<BinOp> + AbstractMonoid<BinOp> + Clone;

    fn lift(val: BinOp::In) -> Self::Partial;
    fn lower(part: &Self::Partial) -> BinOp::Out;
}

pub trait AggregateGroup<BinOp: AggregateOperator> {
    type Partial: Identity<BinOp> + AbstractGroup<BinOp> + Clone;

    fn lift(val: BinOp::In) -> Self::Partial;
    fn lower(part: &Self::Partial) -> BinOp::Out;
}

pub mod max;

pub mod mean;

pub mod sum;
