use std::sync::atomic::Ordering;
use std::sync::atomic::fence;
use std::time::Instant;

use num_traits::{NumAssign, Zero};
use num_traits::cast::NumCast;

use clap::Clap;

use swag::FifoWindow;
use swag::reactive::Reactive;
use swag::recalc::ReCalc;
use swag::soe::SoE;
use swag::two_stacks::TwoStacks;
use swag::two_stacks_lite::TwoStacksLite;
use swag::ops::AggregateOperator;
use swag::ops::max::Max;
use swag::ops::mean::Mean;
use swag::ops::sum::Sum;

#[derive(Clap)]
struct Opts {
    // cargo calls us with --bench, so we just need to consume it
    #[clap(long, parse(try_from_str), default_value="true")]
    _bench: String,

    // cargo passes our own name, so we just need to consume it
    _name: String,

    swag: String,
    function: String,
    window_size: usize,
    iterations: usize,

    #[clap(long, parse(try_from_str), default_value="false")]
    latency: bool
}

fn static_core<T, BinOp, Window>(opts: &Opts)
where
    T: NumAssign + NumCast + std::fmt::Display,
    Window: FifoWindow<BinOp>,
    BinOp: AggregateOperator<In=T, Out=T>
{
    let mut window = Window::new();
    let mut force_side_effect: T = Zero::zero();

    for i in 0..opts.window_size {
        window.push(NumCast::from(1 + (i % 101)).unwrap());
    }

    assert_eq!(window.len(), opts.window_size);

    let start = Instant::now();

    for i in opts.window_size..opts.iterations {
        fence(Ordering::SeqCst);

        window.pop();
        window.push(NumCast::from(1 + (i % 101)).unwrap());
        force_side_effect += window.query();
    }

    let duration = start.elapsed();
    println!("core runtime: {:?}", duration);
    println!("{}", force_side_effect);
}

macro_rules! query_run {
    {
        $opts:ident, $num:ty => [$( [$swag:ident, $($function:ident),+] ),*]
    } => {
            {
                $(
                    $(
                        if $opts.swag == $swag::<$function::<$num, $num>>::name() {
                            if $opts.function == $function::<$num, $num>::name() {
                                static_core::<$num, $function::<$num, $num>, $swag::<$function::<$num, $num>>>(&$opts);
                                std::process::exit(0);
                            }
                        }
                    )*
                )*
            }
    }
}

fn main() {
    let opts: Opts = Opts::parse();

    println!("agg {}, fun {}, win {}, it {}, lat {}", opts.swag, 
                                                      opts.function, 
                                                      opts.window_size, 
                                                      opts.iterations, 
                                                      opts.latency);
    query_run! {
        opts, i32 =>
            [[ReCalc,        Max, Mean, Sum],
             [TwoStacks,     Max, Mean, Sum],
             [TwoStacksLite, Max, Mean, Sum],
             [Reactive,      Max, Mean, Sum],
             [SoE,           Mean, Sum]]
    }

    // Should not reach here
    panic!("error: unrecognized combination of swag ({}) and function ({})", opts.swag, opts.function);
}
