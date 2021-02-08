use std::cmp::PartialEq;
use std::ops::{Add, Neg};
use std::marker::PhantomData;

use num_traits::Zero;
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

use super::AggregateOperator;
use super::AggregateMonoid;
use super::AggregateGroup;

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


