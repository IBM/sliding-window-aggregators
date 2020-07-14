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

/// An integer value
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub struct Int(pub i32);

/// Binary operator for calculating the arithmetic sum.
/// Has the following properties:
/// * Invertibility
/// * Associativity
/// * Commutativity
#[derive(Copy, Clone)]
pub struct Sum;

impl Operator for Sum {
    fn operator_token() -> Sum {
        Sum
    }
}

impl Identity<Sum> for Int {
    fn identity() -> Int {
        Int(0)
    }
}

impl AbstractMagma<Sum> for Int {
    fn operate(&self, other: &Self) -> Self {
        Int(self.0 + other.0)
    }
}

impl TwoSidedInverse<Sum> for Int {
    fn two_sided_inverse(&self) -> Int {
        Int(-self.0)
    }
}

impl AbstractSemigroup<Sum> for Int {}
impl AbstractMonoid<Sum> for Int {}
impl AbstractQuasigroup<Sum> for Int {}
impl AbstractLoop<Sum> for Int {}
impl AbstractGroup<Sum> for Int {}

/// Binary operator for calculating the maximum Int.
/// Has the following properties:
/// * Associativity
/// * Commutativity
#[derive(Copy, Clone)]
pub(crate) struct Max;

impl Operator for Max {
    fn operator_token() -> Max {
        Max
    }
}

impl Identity<Max> for Int {
    fn identity() -> Int {
        Int(std::i32::MIN)
    }
}

impl AbstractMagma<Max> for Int {
    fn operate(&self, other: &Self) -> Self {
        if self.0 > other.0 {
            *self
        } else {
            *other
        }
    }
}

impl AbstractSemigroup<Max> for Int {}
impl AbstractMonoid<Max> for Int {}
