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
                        super::$name::<swag::$module::$algorithm<_,>>();
                    }
                )*
            }
        )*
    }
}

/// Basic test for integer sums.
fn test1<Window>()
where
    Window: FifoWindow<Sum<i32, i32>>,
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
    Window: FifoWindow<Sum<i32, i32>>,
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
    Window: FifoWindow<Max<i32, i32>>,
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
    Window: FifoWindow<Sum<i32, i32>>,
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
    Window: FifoWindow<Sum<i32, i32>>,
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
    Window: FifoWindow<Sum<i32, i32>>,
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
    Window: FifoWindow<Sum<i32, i32>>,
{
    let mut window = Window::new();
    window.push(1);
    assert_eq!(window.query(), 1);
}

/// Push => Push => Push => Pop => Pop => Query
fn test8<Window>()
where
    Window: FifoWindow<Sum<i32, i32>>,
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
    Window: FifoWindow<Sum<i32, i32>>,
{
    let mut window = Window::new();
    window.push(1);
    window.pop();
    window.push(2);
    assert_eq!(window.query(), 2);
}

fn test_max<Window>()
where
    Window: FifoWindow<Max<i32, i32>>
{
    let mut window = Window::new();
    let top = 1000;
    for i in 0..=top {
        window.push(i);
        assert_eq!(window.query(), i);
    }

    for _i in 0..=top {
        assert_eq!(window.query(), top);
        window.pop();
    }
}

fn test_sum<Window>()
where
    Window: FifoWindow<Sum<i32, i32>>
{
    let mut window = Window::new();
    let top = 1000;
    let mut running_sum = 0;
    for i in 0..=top {
        window.push(i);
        running_sum += i;
        assert_eq!(window.query(), running_sum);
    }

    for i in 0..=top {
        window.pop();
        running_sum -= i;
        assert_eq!(window.query(), running_sum);
    }
}

test_matrix! {
    test1 =>    [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test2 =>    [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test3 =>    [ recalc::ReCalc,           reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test4 =>    [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test5 =>    [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test6 =>    [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test7 =>    [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test8 =>    [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test9 =>    [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test_max => [ recalc::ReCalc,           reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ],
    test_sum => [ recalc::ReCalc, soe::SoE, reactive::Reactive, two_stacks::TwoStacks, flatfit::FlatFIT ]
}

#[test]
fn assert_names() {
    assert_eq!(recalc::ReCalc::<Sum::<i32, i32>>::name(), "recalc");
    assert_eq!(flatfit::FlatFIT::<Sum::<i32, i32>>::name(), "flatfit");
    assert_eq!(reactive::Reactive::<Sum::<i32, i32>>::name(), "reactive");
    assert_eq!(two_stacks::TwoStacks::<Sum::<i32, i32>>::name(), "two_stacks");
    assert_eq!(soe::SoE::<Sum::<i32, i32>>::name(), "soe");
}

