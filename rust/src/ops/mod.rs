use alga::general::AbstractGroup;
use alga::general::AbstractLoop;
use alga::general::AbstractMagma;
use alga::general::AbstractMonoid;
use alga::general::AbstractQuasigroup;
use alga::general::AbstractSemigroup;
use alga::general::Identity;
use alga::general::Operator;
use alga::general::TwoSidedInverse;
use num_traits::{Num, Zero};
use num_traits::cast::{ToPrimitive, NumCast};
use std::marker::PhantomData;
use std::fmt;

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

pub trait MonoidAdapter<In, Out> {
    type Partial;

    fn lift(&self, val: In) -> Self::Partial;
    fn lower(&self, part: &Self::Partial) -> Out;
}

/// Binary operator for arithmetic sum.
/// Has the following properties:
/// * Invertibility
/// * Associativity
/// * Commutativity
#[derive(Copy, Clone)]
pub struct Sum;

impl Sum {
    pub fn name() -> &'static str {
        "sum"
    }
}

impl fmt::Display for Sum {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", Sum::name())
    }
}

impl Operator for Sum {
    fn operator_token() -> Sum {
        Sum
    }
}

impl Identity<Sum> for i32 {
    fn identity() -> i32 {
        0
    }
}

impl AbstractMagma<Sum> for i32 {
    fn operate(&self, other: &Self) -> Self {
        self + other
    }
}

impl TwoSidedInverse<Sum> for i32 {
    fn two_sided_inverse(&self) -> i32 {
        -self
    }
}

impl AbstractSemigroup<Sum> for i32 {}
impl AbstractMonoid<Sum> for i32 {}
impl AbstractQuasigroup<Sum> for i32 {}
impl AbstractLoop<Sum> for i32 {}
impl AbstractGroup<Sum> for i32 {}

/// Binary operator for maximum.
/// Has the following properties:
/// * Associativity
/// * Commutativity
#[derive(Copy, Clone)]
pub struct Max;

impl Max {
    pub fn name() -> &'static str {
        "max"
    }
}

impl fmt::Display for Max {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", Max::name())
    }
}

impl Operator for Max {
    fn operator_token() -> Max {
        Max
    }
}

impl Identity<Max> for i32 {
    fn identity() -> i32 {
        std::i32::MIN
    }
}

impl AbstractMagma<Max> for i32 {
    fn operate(&self, other: &Self) -> Self {
        if self > other {
            *self
        } else {
            *other
        }
    }
}

impl AbstractSemigroup<Max> for i32 {}
impl AbstractMonoid<Max> for i32 {}

/// Binary operator for mean.
/// Has the following properties:
/// * Associativity
/// * Commutativity

#[derive(Copy, Clone)]
pub struct Mean<In, Out>
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{
    in_type: PhantomData<In>,
    out_type: PhantomData<Out>,
}

#[derive(Copy, Clone, Eq, PartialEq)]
pub struct MeanPartial<T> {
    sum: T,
    n: usize,
}

impl<In, Out> MonoidAdapter<In, Out> for Mean<In, Out>
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{
    type Partial = MeanPartial<In>;

    fn lift(&self, val: In) -> MeanPartial<In> {
        MeanPartial::<In>{sum: val, n: 1}
    }
    fn lower(&self, part: &Self::Partial) -> Out {
        NumCast::from(part.sum.to_usize().unwrap() / part.n).unwrap()
    }
}

impl<In, Out> Mean<In, Out> 
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{
    pub fn name() -> &'static str {
        "mean"
    }
}

impl<In, Out> fmt::Display for Mean<In, Out> 
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", Mean::<In, Out>::name())
    }
}

impl<In, Out> Operator for Mean<In, Out> 
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{
    fn operator_token() -> Self {
        Mean::<In, Out>{in_type: PhantomData, out_type: PhantomData}
    }
}

impl<In, Out> Identity<Mean::<In, Out>> for MeanPartial<In>
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{
    fn identity() -> MeanPartial<In> {
        MeanPartial::<In>{sum: Zero::zero(), n: 0}
    }
}

impl<In, Out> AbstractMagma<Mean::<In, Out>> for MeanPartial<In>
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{
    fn operate(&self, other: &Self) -> Self {
        MeanPartial::<In>{sum: self.sum + other.sum,
                          n: self.n + other.n}
    }
}

impl<In, Out> AbstractSemigroup<Mean::<In, Out>> for MeanPartial<In>
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{}

impl<In, Out> AbstractMonoid<Mean::<In, Out>> for MeanPartial<In>
where
    In: Num + ToPrimitive + Copy,
    Out: Num + NumCast + Copy,
{}

