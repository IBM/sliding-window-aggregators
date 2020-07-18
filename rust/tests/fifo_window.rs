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

fn synthesize(size: usize) -> Vec<Int> {
    let mut rng = rand::thread_rng();
    (0..size)
        .map(|_| rng.gen_range(1, 5))
        .map(Int)
        .collect::<Vec<_>>()
}

/// Tries to aggregate the sum of 1K randomly generated integers.
fn test2<Window>()
where
    Window: FifoWindow<Int, Sum>,
{
    let values = synthesize(1_000);
    let sum = values.iter().fold(0, |acc, Int(x)| acc + x);
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

/// Tries to find the maximum value out 1K randomly generated integers.
fn test3<Window>()
where
    Window: FifoWindow<Int, Max>,
{
    let mut window = Window::new();
    let values = synthesize(1_000);
    let max = values.iter().map(|Int(x)| *x).max().unwrap();
    for v in values.clone() {
        window.push(v);
    }
    assert_eq!(window.query(), Int(max));
    for _ in values {
        window.pop();
    }
    assert_eq!(window.query(), Int(std::i64::MIN));
}

/// Fills a window with 1K elements and pushes/pops/queries 1K times.
fn test4<Window>()
where
    Window: FifoWindow<Int, Sum>,
{
    let mut window = Window::new();
    let values = synthesize(1_000);
    let sum = values.iter().fold(0, |acc, Int(x)| acc + x);
    for v in values.clone() {
        window.push(v);
    }
    for v in values {
        window.push(v);
        window.pop();
        assert_eq!(window.query(), Int(sum));
    }
}

#[test]
fn test1_recalc() {
    test1::<ReCalc<_, _>>();
}

#[test]
fn test2_recalc() {
    test2::<ReCalc<_, _>>();
}

#[test]
fn test3_recalc() {
    test3::<ReCalc<_, _>>();
}

#[test]
fn test4_recalc() {
    test4::<ReCalc<_, _>>();
}

#[test]
fn test1_soe() {
    test1::<SoE<_, _>>();
}

#[test]
fn test2_soe() {
    test2::<SoE<_, _>>();
}

#[test]
fn test4_soe() {
    test4::<SoE<_, _>>();
}

#[test]
fn test1_two_stacks() {
    test1::<TwoStacks<_, _>>();
}

#[test]
fn test2_two_stacks() {
    test2::<TwoStacks<_, _>>();
}

#[test]
fn test3_two_stacks() {
    test3::<TwoStacks<_, _>>();
}

#[test]
fn test4_two_stacks() {
    test4::<TwoStacks<_, _>>();
}

#[test]
fn test1_reactive() {
    test1::<Reactive<_, _>>();
}

#[test]
fn test2_reactive() {
    test2::<Reactive<_, _>>();
}

#[test]
fn test3_reactive() {
    test3::<Reactive<_, _>>();
}

#[test]
fn test4_reactive() {
    test4::<Reactive<_, _>>();
}
