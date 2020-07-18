use rand::Rng;
use swag::*;

mod common;
use common::*;

/// Macro for generating test cases for different algorithms.
macro_rules! test_matrix {
    {
        $(
            $name:ident => [$($module:ident::$algorithm:ident),*]
        ),*
    } => {
        $(
            mod $name {
                $(
                    #[test]
                    fn $module() {
                        super::$name::<swag::$module::$algorithm<_,_>>();
                    }
                )*
            }
        )*
    }
}

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
        window.query();
        assert_eq!(window.query(), Int(sum));
    }
}

/// Pops more elements from a window than what it contains.
fn test5<Window>()
where
    Window: FifoWindow<Int, Sum>,
{
    let mut window = Window::new();
    window.push(Int(0));
    window.push(Int(0));
    window.pop();
    window.pop();
    window.pop();
}

/// Pops more elements from a window than what it contains.
fn test6<Window>()
where
    Window: FifoWindow<Int, Sum>,
{
    let mut window = Window::new();
    window.push(Int(1));
    window.push(Int(2));
    window.push(Int(3));
    window.pop();
    window.push(Int(4));
    window.push(Int(5));
}

test_matrix! {
    test1 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks ],
    test2 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks ],
    test3 => [ recalc::ReCalc,           reactive::Reactive, two_stacks::TwoStacks ],
    test4 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks ],
    test5 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks ],
    test6 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks ]
}
