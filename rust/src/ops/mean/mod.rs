use std::cmp::PartialEq;
use std::ops::Neg;
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

use super::AggregateOperator;
use super::AggregateMonoid;
use super::AggregateGroup;

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

