use rand::Rng;
use swag::*;
use swag::ops::*;

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
    Window: FifoWindow<i32, Sum>,
{
    let mut window = Window::new();

    assert_eq!(window.query(), 0);

    window.push(1);

    assert_eq!(window.query(), 1);

    window.push(2);

    assert_eq!(window.query(), 3);

    window.push(3);

    assert_eq!(window.query(), 6);

    window.pop();

    assert_eq!(window.query(), 5);
}

fn synthesize(size: usize) -> Vec<i32> {
    let mut rng = rand::thread_rng();
    (0..size)
        .map(|_| rng.gen_range(1, 5))
        .collect::<Vec<_>>()
}

/// Tries to aggregate the sum of 1K randomly generated integers.
fn test2<Window>()
where
    Window: FifoWindow<i32, Sum>,
{
    let values = synthesize(1_000);
    let sum = values.iter().fold(0, |acc, x| acc + x);
    let mut window = Window::new();
    for v in values.clone() {
        window.push(v);
    }
    assert_eq!(window.query(), sum);
    for _ in values {
        window.pop();
    }
    assert_eq!(window.query(), 0);
}

/// Tries to find the maximum value out 1K randomly generated integers.
fn test3<Window>()
where
    Window: FifoWindow<i32, Max>,
{
    let mut window = Window::new();
    let values = synthesize(1_000);
    let max = values.iter().map(|x| *x).max().unwrap();
    for v in values.clone() {
        window.push(v);
    }
    assert_eq!(window.query(), max);
    for _ in values {
        window.pop();
    }
    assert_eq!(window.query(), std::i32::MIN);
}

/// Fills a window with 1K elements and pushes/pops/queries 1K times.
fn test4<Window>()
where
    Window: FifoWindow<i32, Sum>,
{
    let mut window = Window::new();
    let values = synthesize(1_000);
    let sum = values.iter().fold(0, |acc, x| acc + x);
    for v in values.clone() {
        window.push(v);
    }
    for v in values {
        window.push(v);
        window.pop();
        assert_eq!(window.query(), sum);
    }
}

/// Pops more elements from a window than what it contains.
fn test5<Window>()
where
    Window: FifoWindow<i32, Sum>,
{
    let mut window = Window::new();
    window.push(0);
    window.push(0);
    window.pop();
    window.pop();
    window.pop();
}

/// Pops more elements from a window than what it contains.
fn test6<Window>()
where
    Window: FifoWindow<i32, Sum>,
{
    let mut window = Window::new();
    window.push(1);
    window.push(2);
    window.push(3);
    window.pop();
    window.push(4);
    window.push(5);
}

/// Push => Query
fn test7<Window>()
where
    Window: FifoWindow<i32, Sum>,
{
    let mut window = Window::new();
    window.push(1);
    assert_eq!(window.query(), 1);
}

/// Push => Push => Push => Pop => Pop => Query
fn test8<Window>()
where
    Window: FifoWindow<i32, Sum>,
{
    let mut window = Window::new();
    window.push(1);
    window.push(2);
    window.push(3);
    window.pop();
    window.pop();
    assert_eq!(window.query(), 3);
}

/// Push => Pop => Push => Query
fn test9<Window>()
where
    Window: FifoWindow<i32, Sum>,
{
    let mut window = Window::new();
    window.push(1);
    window.pop();
    window.push(2);
    assert_eq!(window.query(), 2);
}

test_matrix! {
    test1 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test2 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test3 => [ recalc::ReCalc,           reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test4 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test5 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test6 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test7 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test8 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test9 => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ]
}
