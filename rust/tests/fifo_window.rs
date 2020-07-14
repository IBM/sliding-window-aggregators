use rand::Rng;
use swag::reactive::*;
use swag::recalc::*;
use swag::soe::*;
use swag::two_stacks::*;
use swag::*;

mod common;
use common::*;

/// Basic test for integer sums.
fn test1<Window>()
where
    Window: FifoWindow<Int, Sum>,
{
    let mut window = Window::new();

    assert_eq!(window.query(), Int(0));

    window.push(Int(1));

    assert_eq!(window.query(), Int(1));

    window.push(Int(2));

    assert_eq!(window.query(), Int(3));

    window.push(Int(3));

    assert_eq!(window.query(), Int(6));

    window.pop();

    assert_eq!(window.query(), Int(5));
}

fn generate() -> Vec<Int> {
    let mut rng = rand::thread_rng();
    (0..1000)
        .map(|_| rng.gen_range(1, 5))
        .map(Int)
        .collect::<Vec<_>>()
}

/// Tries to aggregate the sum of 1000 randomly generated integers.
fn test2<Window>()
where
    Window: FifoWindow<Int, Sum>,
{
    let values = generate();
    let sum: i32 = values.iter().fold(0, |acc, Int(x)| acc + x);
    let mut window = Window::new();
    for v in values.clone() {
        window.push(v);
    }
    assert_eq!(window.query(), Int(sum));
    for _ in values {
        window.pop();
    }
    assert_eq!(window.query(), Int(0));
}

/// Tries to find the maximum value out 1000 randomly generated integers.
fn test3<Window>()
where
    Window: FifoWindow<Int, Max>,
{
    let mut window = Window::new();
    let values = generate();
    let max = values.iter().map(|Int(x)| *x).max().unwrap();
    for v in values.clone() {
        window.push(v);
    }
    assert_eq!(window.query(), Int(max));
    for _ in values {
        window.pop();
    }
    assert_eq!(window.query(), Int(std::i32::MIN));
}

#[test]
fn test1_recalc() {
    test1::<ReCalc<Int, Sum>>();
}

#[test]
fn test2_recalc() {
    test2::<ReCalc<Int, Sum>>();
}

#[test]
fn test3_recalc() {
    test3::<ReCalc<Int, Max>>();
}

#[test]
fn test1_soe() {
    test1::<SoE<Int, Sum>>();
}

#[test]
fn test2_soe() {
    test2::<SoE<Int, Sum>>();
}

#[test]
fn test1_two_stacks() {
    test1::<TwoStacks<Int, Sum>>();
}

#[test]
fn test2_two_stacks() {
    test2::<TwoStacks<Int, Sum>>();
}

#[test]
fn test3_two_stacks() {
    test3::<TwoStacks<Int, Max>>();
}

#[test]
fn test1_reactive() {
    test1::<Reactive<Int, Sum>>();
}

#[test]
fn test2_reactive() {
    test2::<Reactive<Int, Sum>>();
}

#[test]
fn test3_reactive() {
    test3::<Reactive<Int, Max>>();
}
