use alga::general::AbstractGroup;
use alga::general::AbstractLoop;
use alga::general::AbstractMagma;
use alga::general::AbstractMonoid;
use alga::general::AbstractQuasigroup;
use alga::general::AbstractSemigroup;
use alga::general::Identity;
use alga::general::Operator;
use alga::general::TwoSidedInverse;
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

/// Binary operator for calculating the arithmetic sum.
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

/// Binary operator for calculating the maximum Int.
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
