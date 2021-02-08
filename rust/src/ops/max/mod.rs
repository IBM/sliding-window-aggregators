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


