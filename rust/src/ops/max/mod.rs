use std::cmp::{Ord, PartialEq};
use std::marker::PhantomData;

use num_traits::cast::{ToPrimitive, NumCast};

use alga::general::AbstractMagma;
use alga::general::AbstractMonoid;
use alga::general::AbstractSemigroup;
use alga::general::Identity;
use alga::general::Operator;

use super::AggregateOperator;
use super::AggregateMonoid;

// We need a generic way to talk about a type's minimum 
// value. Unfortunately, we'll need a new implementation 
// for every type we want to support. I don't know of a 
// way around this yet.
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
pub struct Max<In, Out> {
    in_type: PhantomData<In>,
    out_type: PhantomData<Out>
}

// We need this because of the orphan rules for trait implemenations. See 
// `rustc --explain E0210`.
#[derive(Copy, Clone, Eq, PartialEq)]
pub struct MaxPartial<T> {
    val: T
}

// Replace the below with proper trait aliases when that feature 
// stabilizes.
pub trait MaxIn: Ord + Min + ToPrimitive + Copy {}
impl<T> MaxIn for T where T: Ord + Min + ToPrimitive + Copy {}

pub trait MaxOut: Ord + Min + NumCast + Copy {}
impl<T> MaxOut for T where T: Ord + Min + NumCast + Copy {}

impl<In: MaxIn, Out: MaxOut> AggregateMonoid<Max<In, Out>> for Max<In, Out> {
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

impl<In_: MaxIn, Out_: MaxOut> AggregateOperator for Max<In_, Out_> {
    type In = In_;
    type Out = Out_;
}

impl<In: MaxIn, Out: MaxOut> Max<In, Out> {
    pub fn name() -> &'static str {
        "max"
    }
}

impl<In: MaxIn, Out: MaxOut> Operator for Max<In, Out> {
    fn operator_token() -> Self {
        Self {
            in_type: PhantomData,
            out_type: PhantomData
        }
    }
}

impl<In: MaxIn, Out: MaxOut> Identity<Max<In, Out>> for MaxPartial<Out> {
    fn identity() -> Self {
        Self {
            val: Min::min()
        }
    }
}

impl<In: MaxIn, Out: MaxOut> AbstractMagma<Max<In, Out>> for MaxPartial<Out> {
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

impl<In: MaxIn, Out: MaxOut> AbstractSemigroup<Max<In, Out>> for MaxPartial<Out> {}
impl<In: MaxIn, Out: MaxOut> AbstractMonoid<Max<In, Out>> for MaxPartial<Out> {}

