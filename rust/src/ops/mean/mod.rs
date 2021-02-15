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
pub struct Mean<In, Out> {
    in_type: PhantomData<In>,
    out_type: PhantomData<Out>,
}

#[derive(Copy, Clone, Eq, PartialEq)]
pub struct MeanPartial<T> {
    sum: T,
    n: T,
}

// Replace the below with proper trait aliases when that feature 
// stabilizes.
pub trait MeanIn: Num + ToPrimitive + Copy {}
impl<T> MeanIn for T where T: Num + ToPrimitive + Copy {}

pub trait MeanOut: Num + NumCast + Copy {}
impl<T> MeanOut for T where T: Num + NumCast + Copy {}

impl<In: MeanIn, Out: MeanOut> AggregateMonoid<Mean<In, Out>> for Mean<In, Out> {
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
    In: MeanIn + Neg<Output=In>,
    Out: MeanOut + Neg<Output=Out>,
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

impl<In_: MeanIn, Out_: MeanOut> AggregateOperator for Mean<In_, Out_> {
    type In = In_;
    type Out = Out_;
}

impl<In: MeanIn, Out: MeanOut> Mean<In, Out> {
    pub fn name() -> &'static str {
        "mean"
    }
}

impl<In: MeanIn, Out: MeanOut> Operator for Mean<In, Out> {
    fn operator_token() -> Self {
        Self {
            in_type: PhantomData, 
            out_type: PhantomData
        }
    }
}

impl<In: MeanIn, Out: MeanOut> Identity<Mean<In, Out>> for MeanPartial<Out> {
    fn identity() -> Self {
        Self {
            sum: Zero::zero(), 
            n: Zero::zero() 
        }
    }
}

impl<In: MeanIn, Out: MeanOut> AbstractMagma<Mean<In, Out>> for MeanPartial<Out> {
    fn operate(&self, other: &Self) -> Self {
        Self {
            sum: self.sum + other.sum,
            n: self.n + other.n
        }
    }
}

impl<In, Out> TwoSidedInverse<Mean<In, Out>> for MeanPartial<Out>
where
    In: MeanIn + Neg<Output=In>,
    Out: MeanOut + Neg<Output=Out>,
{
    fn two_sided_inverse(&self) -> Self {
        Self {
            sum: -self.sum,
            n: -self.n
        }
    }
}

impl<In: MeanIn, Out: MeanOut> AbstractSemigroup<Mean<In, Out>> for MeanPartial<Out> {}
impl<In: MeanIn, Out: MeanOut> AbstractMonoid<Mean<In, Out>> for MeanPartial<Out> {}

impl<In: MeanIn + Neg<Output=In>, Out: MeanOut + Neg<Output=Out>> AbstractQuasigroup<Mean<In, Out>> for MeanPartial<Out> {}
impl<In: MeanIn + Neg<Output=In>, Out: MeanOut + Neg<Output=Out>> AbstractLoop<Mean<In, Out>> for MeanPartial<Out> {}
impl<In: MeanIn + Neg<Output=In>, Out: MeanOut + Neg<Output=Out>> AbstractGroup<Mean<In, Out>> for MeanPartial<Out> {}

