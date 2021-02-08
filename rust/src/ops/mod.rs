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

/// An operator used in an aggregation. Unlike an `Operator`, an 
/// `AggregateOperator` can have different `In` and `Out` types.
///
/// Note: It is tempting to try to eliminate this trait, and fold the `In`
///       and `Out` types into `AggregateMonoid` and `AggregateGroup`. That
///       does not work because these types are a part of the public interface
///       for `FifoWindow`, whereas everything in `AggregateMonoid` and 
///       `AggregateGroup` is not.
pub trait AggregateOperator: Operator {
    type In: Clone;
    type Out: Clone;
}

/// A monoid that can be used in an aggregation. The monoid is over a `Partial` 
/// type, which must also have an identity. The `lift` and `lower` helper 
/// functions convert the binary operator's `In` and `Out` types to and from 
/// the `Partial` type.
pub trait AggregateMonoid<BinOp: AggregateOperator>  {
    type Partial: Identity<BinOp> + AbstractMonoid<BinOp> + Clone;

    fn lift(val: BinOp::In) -> Self::Partial;
    fn lower(part: &Self::Partial) -> BinOp::Out;
}
/// A group that can be used in an aggregation. The monoid is over a `Partial` 
/// type, which must also have an identity. The `lift` and `lower` helper 
/// functions convert the binary operator's `In` and `Out` types to and from 
/// the `Partial` type. Note that to be a group, the binary operator must be 
/// invertible, which means the `Partial` type must have an implementation for
/// `TwoSidedInverse`.
pub trait AggregateGroup<BinOp: AggregateOperator> {
    type Partial: Identity<BinOp> + AbstractGroup<BinOp> + Clone;

    fn lift(val: BinOp::In) -> Self::Partial;
    fn lower(part: &Self::Partial) -> BinOp::Out;
}

pub mod max;

pub mod mean;

pub mod sum;
