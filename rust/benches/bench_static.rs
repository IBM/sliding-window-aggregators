use std::sync::atomic::Ordering;
use std::sync::atomic::fence;
use std::time::Instant;

use clap::Clap;

use swag::reactive::Reactive;
use swag::recalc::ReCalc;
use swag::soe::SoE;
use swag::two_stacks::TwoStacks;
use swag::FifoWindow;
use swag::ops::AggregateOperator;
use swag::ops::Sum;
use swag::ops::Max;

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

fn static_core<BinOp, Window>(opts: &Opts)
where
    Window: FifoWindow<BinOp>,
    BinOp: AggregateOperator<In=i32, Out=i32>
{
    let mut window = Window::new();
    let mut force_side_effect = 0;

    for i in 0..opts.window_size {
        window.push((1 + (i % 101)) as i32);
    }

    assert_eq!(window.len(), opts.window_size);

    let start = Instant::now();

    for i in opts.window_size..opts.iterations {
        fence(Ordering::SeqCst);

        window.pop();
        window.push((1 + (i % 101)) as i32);
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
                                static_core::<$function::<$num, $num>, $swag::<$function::<$num, $num>>>(&$opts);
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
            [[ReCalc, Sum, Max],
             [TwoStacks, Sum, Max],
             [Reactive, Sum, Max],
             [SoE, Sum]]
    }

    // Should not reach here
    eprintln!("error: unrecognized swag ({}) or function ({})", opts.swag, opts.function);
    std::process::exit(1);
}
