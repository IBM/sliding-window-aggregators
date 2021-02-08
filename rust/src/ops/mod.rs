use std::cmp::{Ord, PartialEq};
use std::ops::{Add, Neg};
use std::marker::PhantomData;

use num_traits::{Num, One, Zero};
use num_traits::cast::{ToPrimitive, NumCast};

use alga::general::AbstractGroup;
use alga::general::AbstractLoop;
use alga::general::AbstractMagma;
use alga::general::AbstractMonoid;
use alga::general::AbstractQuasigroup;
use alga::general::AbstractSemigroup;
use alga::general::Identity;
use alga::general::Operator;
use alga::general::TwoSidedInverse;

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

/// Binary operator for arithmetic sum.
/// Has the following properties:
/// * Invertibility
/// * Associativity
/// * Commutativity
#[derive(Copy, Clone)]
pub struct Sum<In, Out> 
where
    In: Add + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + Neg<Output=Out> + NumCast + Zero + Copy,
{
    in_type: PhantomData<In>,
    out_type: PhantomData<Out>
}

impl<In, Out> Sum<In, Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + Zero + Copy,
{
    pub fn name() -> &'static str {
        "sum"
    }
}

impl<In_, Out_> AggregateOperator for Sum<In_, Out_>
where
    In_: Add + PartialEq + Neg<Output=In_> + ToPrimitive + Zero + Copy,
    Out_: Add + PartialEq + Neg<Output=Out_> + NumCast + Zero + Copy,
{
    type In = In_;
    type Out = Out_;
}

// We need this because of the orphan rules for trait implemenations. See 
// `rustc --explain E0210`.
#[derive(Copy, Clone, Eq, PartialEq)]
pub struct SumPartial<T> {
    val: T
}

impl<In, Out> AggregateMonoid<Sum<In, Out>> for Sum<In, Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + Zero + Copy,
{
    type Partial = SumPartial<Out>;

    fn lift(v: In) -> Self::Partial {
        Self::Partial {
            val: NumCast::from(v).unwrap()
        }
    }
    fn lower(part: &Self::Partial) -> Out {
        part.val 
    }
}

impl<In, Out> AggregateGroup<Sum<In, Out>> for Sum<In, Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + Zero + Copy,
{
    type Partial = SumPartial<Out>;

    fn lift(v: In) -> Self::Partial {
        Self::Partial {
            val: NumCast::from(v).unwrap()
        }
    }
    fn lower(part: &Self::Partial) -> Out {
        part.val 
    }
}

impl<In, Out> Operator for Sum<In, Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{
    fn operator_token() -> Self {
        Self {
            in_type: PhantomData,
            out_type: PhantomData
        }
    }
}

impl<In, Out> Identity<Sum<In, Out>> for SumPartial<Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{
    fn identity() -> Self {
        Self {
            val: Zero::zero()
        }
    }
}

impl<In, Out> AbstractMagma<Sum<In, Out>> for SumPartial<Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{
    fn operate(&self, other: &Self) -> Self {
        Self {
            val: self.val + other.val
        }
    }
}

impl<In, Out> TwoSidedInverse<Sum<In, Out>> for SumPartial<Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{
    fn two_sided_inverse(&self) -> Self {
        Self {
            val: -(self.val)
        }
    }
}

impl<In, Out> AbstractSemigroup<Sum<In, Out>> for SumPartial<Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{}

impl<In, Out> AbstractMonoid<Sum<In, Out>> for SumPartial<Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{}

impl<In, Out> AbstractQuasigroup<Sum<In, Out>> for SumPartial<Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{}

impl<In, Out> AbstractLoop<Sum<In, Out>> for SumPartial<Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{}

impl<In, Out> AbstractGroup<Sum<In, Out>> for SumPartial<Out>
where
    In: Add + PartialEq + Neg<Output=In> + ToPrimitive + Zero + Copy,
    Out: Add + PartialEq + Neg<Output=Out> + NumCast + ToPrimitive + Zero + Copy
{}

// We need a generic way to talk about a type's minimum 
// value
pub trait Min {
    fn min() -> Self;
}

impl Min for i32 {
    fn min() -> i32 {
        i32::MIN
    }
}

/// Binary operator for maximum.
/// Has the following properties:
/// * Associativity
/// * Commutativity
#[derive(Copy, Clone)]
pub struct Max<In, Out>
where
    In: Ord + Min + Copy,
    Out: Ord + Min + Copy,
{
    in_type: PhantomData<In>,
    out_type: PhantomData<Out>
}

// We need this because of the orphan rules for trait implemenations. See 
// `rustc --explain E0210`.
#[derive(Copy, Clone, Eq, PartialEq)]
pub struct MaxPartial<T> {
    val: T
}

impl<In, Out> AggregateMonoid<Max<In, Out>> for Max<In, Out>
where
    In: Ord + Min + ToPrimitive + Copy,
    Out: Ord + Min + NumCast + Copy,
{
    type Partial = MaxPartial<Out>;

    fn lift(v: In) -> Self::Partial {
        Self::Partial{
            val: NumCast::from(v).unwrap()
        }
    }
    fn lower(part: &Self::Partial) -> Out {
        part.val
    }
}

impl<In_, Out_> AggregateOperator for Max<In_, Out_>
where
    In_: Ord + Min + ToPrimitive + Copy,
    Out_: Ord + Min + NumCast + Copy,
{
    type In = In_;
    type Out = Out_;
}

impl<T: Ord + Min + NumCast + ToPrimitive + Copy> Max<T, T> {
    pub fn name() -> &'static str {
        "max"
    }
}

impl<In, Out> Operator for Max<In, Out> 
where
    In: Ord + Min + ToPrimitive + Copy,
    Out: Ord + Min + NumCast + Copy,
{
    fn operator_token() -> Self {
        Self {
            in_type: PhantomData,
            out_type: PhantomData
        }
    }
}

impl<In, Out> Identity<Max<In, Out>> for MaxPartial<Out>
where
    In: Ord + Min + ToPrimitive + Copy,
    Out: Ord + Min + NumCast + Copy,
{
    fn identity() -> Self {
        Self {
            val: Min::min()
        }
    }
}

impl<In, Out> AbstractMagma<Max<In, Out>> for MaxPartial<Out>
where
    In: Ord + Min + ToPrimitive + Copy,
    Out: Ord + Min + NumCast + Copy,
{
    fn operate(&self, other: &Self) -> Self {
        if self.val > other.val {
            Self {
                val: self.val
            }
        } else {
            Self {
                val: other.val
            }
        }
    }
}

impl<In, Out> AbstractSemigroup<Max<In, Out>> for MaxPartial<Out>
where
    In: Ord + Min + ToPrimitive + Copy,
    Out: Ord + Min + NumCast + Copy,
{}

impl<In, Out> AbstractMonoid<Max<In, Out>> for MaxPartial<Out>
where
    In: Ord + Min + ToPrimitive + Copy,
    Out: Ord + Min + NumCast + Copy,
{}

/// Binary operator for mean.
/// Has the following properties:
/// * Associativity
/// * Commutativity

#[derive(Copy, Clone)]
pub struct Mean<In, Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{
    in_type: PhantomData<In>,
    out_type: PhantomData<Out>,
}

#[derive(Copy, Clone, Eq, PartialEq)]
pub struct MeanPartial<T> {
    sum: T,
    n: T,
}

impl<In, Out> AggregateMonoid<Mean<In, Out>> for Mean<In, Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{
    type Partial = MeanPartial<Out>;

    fn lift(val: In) -> MeanPartial<Out> {
        MeanPartial::<Out> {
            sum: NumCast::from(val).unwrap(), 
            n: One::one()
        }
    }
    fn lower(part: &Self::Partial) -> Out {
        part.sum / part.n
    }
}

impl<In, Out> AggregateGroup<Mean<In, Out>> for Mean<In, Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{
    type Partial = MeanPartial<Out>;

    fn lift(val: In) -> MeanPartial<Out> {
        MeanPartial::<Out> {
            sum: NumCast::from(val).unwrap(), 
            n: One::one()
        }
    }
    fn lower(part: &Self::Partial) -> Out {
        part.sum / part.n
    }
}

impl<In_, Out_> AggregateOperator for Mean<In_, Out_>
where
    In_: Num + Neg<Output=In_> + ToPrimitive + Copy,
    Out_: Num + Neg<Output=Out_> + NumCast + Copy,
{
    type In = In_;
    type Out = Out_;
}

impl<In, Out> Mean<In, Out> 
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{
    pub fn name() -> &'static str {
        "mean"
    }
}

impl<In, Out> Operator for Mean<In, Out> 
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{
    fn operator_token() -> Self {
        Self {
            in_type: PhantomData, 
            out_type: PhantomData
        }
    }
}

impl<In, Out> Identity<Mean<In, Out>> for MeanPartial<Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{
    fn identity() -> Self {
        Self {
            sum: Zero::zero(), 
            n: Zero::zero() 
        }
    }
}

impl<In, Out> AbstractMagma<Mean<In, Out>> for MeanPartial<Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{
    fn operate(&self, other: &Self) -> Self {
        Self {
            sum: self.sum + other.sum,
            n: self.n + other.n
        }
    }
}

impl<In, Out> TwoSidedInverse<Mean<In, Out>> for MeanPartial<Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{
    fn two_sided_inverse(&self) -> Self {
        Self {
            sum: -self.sum,
            n: -self.n
        }
    }
}

impl<In, Out> AbstractSemigroup<Mean<In, Out>> for MeanPartial<Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{}

impl<In, Out> AbstractMonoid<Mean<In, Out>> for MeanPartial<Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{}

impl<In, Out> AbstractQuasigroup<Mean<In, Out>> for MeanPartial<Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{}

impl<In, Out> AbstractLoop<Mean<In, Out>> for MeanPartial<Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{}

impl<In, Out> AbstractGroup<Mean<In, Out>> for MeanPartial<Out>
where
    In: Num + Neg<Output=In> + ToPrimitive + Copy,
    Out: Num + Neg<Output=Out> + NumCast + Copy,
{}

