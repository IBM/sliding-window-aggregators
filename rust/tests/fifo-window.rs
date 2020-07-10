use alga::general::AbstractGroup;
use alga::general::AbstractLoop;
use alga::general::AbstractMagma;
use alga::general::AbstractMonoid;
use alga::general::AbstractQuasigroup;
use alga::general::AbstractSemigroup;
use alga::general::Identity;
use alga::general::Operator;
use alga::general::TwoSidedInverse;
use rand::Rng;
use swag::reactive::*;
use swag::recalc::*;
use swag::soe::*;
use swag::two_stacks::*;
use swag::*;

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
struct Value(i32);

#[derive(Copy, Clone, Debug)]
struct Sum;

impl Operator for Sum {
    fn operator_token() -> Sum {
        Sum
    }
}

impl Identity<Sum> for Value {
    fn identity() -> Value {
        Value(0)
    }
}

impl AbstractMagma<Sum> for Value {
    fn operate(&self, other: &Self) -> Self {
        Value(self.0 + other.0)
    }
}

impl TwoSidedInverse<Sum> for Value {
    fn two_sided_inverse(&self) -> Value {
        Value(-self.0)
    }
}

impl AbstractSemigroup<Sum> for Value {}
impl AbstractMonoid<Sum> for Value {}
impl AbstractQuasigroup<Sum> for Value {}
impl AbstractLoop<Sum> for Value {}
impl AbstractGroup<Sum> for Value {}

fn test1<Window>()
where
    Window: FifoWindow<Value, Sum>,
{
    let mut window = Window::new();

    assert_eq!(window.query(), Value(0));

    window.push(Value(1));

    assert_eq!(window.query(), Value(1));

    window.push(Value(2));

    assert_eq!(window.query(), Value(3));

    window.push(Value(3));

    assert_eq!(window.query(), Value(6));

    window.pop();

    assert_eq!(window.query(), Value(5));
}

fn test2<Window>()
where
    Window: FifoWindow<Value, Sum>,
{
    let mut window = Window::new();
    let mut rng = rand::thread_rng();
    let numbers: Vec<i32> = (0..1000).map(|_| rng.gen_range(1, 5)).collect();
    let sum: i32 = numbers.iter().sum();
    let values = numbers.into_iter().map(Value).collect::<Vec<Value>>();
    for v in values.clone() {
        window.push(v);
    }
    assert_eq!(window.query(), Value(sum));
    for _ in values {
        window.pop();
    }
    assert_eq!(window.query(), Value(0));
}

#[test]
fn test1_recalc() {
    test1::<ReCalc<Value, Sum>>();
}

#[test]
fn test2_recalc() {
    test2::<ReCalc<Value, Sum>>();
}

#[test]
fn test1_soe() {
    test1::<SoE<Value, Sum>>();
}

#[test]
fn test2_soe() {
    test2::<SoE<Value, Sum>>();
}

#[test]
fn test1_two_stacks() {
    test1::<TwoStacks<Value, Sum>>();
}

#[test]
fn test2_two_stacks() {
    test2::<TwoStacks<Value, Sum>>();
}

#[test]
fn test1_reactive() {
    test1::<Reactive<Value, Sum>>();
}

#[test]
fn test2_reactive() {
    test2::<Reactive<Value, Sum>>();
}
