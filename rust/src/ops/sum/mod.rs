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
pub struct Sum<In, Out> {
    in_type: PhantomData<In>,
    out_type: PhantomData<Out>
}

impl<In, Out> Sum<In, Out> {
    pub fn name() -> &'static str {
        "sum"
    }
}

// Replace the below with proper trait aliases when that feature 
// stabilizes.
pub trait SumIn: Add + PartialEq + ToPrimitive + Copy {}
impl<T> SumIn for T where T: Add + PartialEq + ToPrimitive + Copy {}

pub trait SumOut: Add + PartialEq + NumCast + Zero + Copy {}
impl<T> SumOut for T where T: Add + PartialEq + NumCast + Zero + Copy {}

impl<In_: SumIn, Out_: SumOut> AggregateOperator for Sum<In_, Out_> {
    type In = In_;
    type Out = Out_;
}

// We need this because of the orphan rules for trait implemenations. See 
// `rustc --explain E0210`.
#[derive(Copy, Clone, Eq, PartialEq)]
pub struct SumPartial<T> {
    val: T
}

impl<In: SumIn, Out: SumOut> AggregateMonoid<Sum<In, Out>> for Sum<In, Out> {
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
    In: SumIn + Neg<Output=In>,
    Out: SumOut + Neg<Output=Out>,
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

impl<In: SumIn, Out: SumOut> Operator for Sum<In, Out> {
    fn operator_token() -> Self {
        Self {
            in_type: PhantomData,
            out_type: PhantomData
        }
    }
}

impl<In: SumIn, Out: SumOut> Identity<Sum<In, Out>> for SumPartial<Out> {
    fn identity() -> Self {
        Self {
            val: Zero::zero()
        }
    }
}

impl<In: SumIn, Out: SumOut> AbstractMagma<Sum<In, Out>> for SumPartial<Out> {
    fn operate(&self, other: &Self) -> Self {
        Self {
            val: self.val + other.val
        }
    }
}

impl<In, Out> TwoSidedInverse<Sum<In, Out>> for SumPartial<Out>
where
    In: SumIn + Neg<Output=In>,
    Out: SumOut + Neg<Output=Out>,
{
    fn two_sided_inverse(&self) -> Self {
        Self {
            val: -self.val
        }
    }
}

impl<In: SumIn, Out: SumOut> AbstractSemigroup<Sum<In, Out>> for SumPartial<Out> {}
impl<In: SumIn, Out: SumOut> AbstractMonoid<Sum<In, Out>> for SumPartial<Out> {}

impl<In: SumIn + Neg<Output=In>, Out: SumOut + Neg<Output=Out>> AbstractQuasigroup<Sum<In, Out>> for SumPartial<Out> {}
impl<In: SumIn + Neg<Output=In>, Out: SumOut + Neg<Output=Out>> AbstractLoop<Sum<In, Out>> for SumPartial<Out> {}
impl<In: SumIn + Neg<Output=In>, Out: SumOut + Neg<Output=Out>> AbstractGroup<Sum<In, Out>> for SumPartial<Out> {}

