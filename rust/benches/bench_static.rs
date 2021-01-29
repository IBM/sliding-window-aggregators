use clap::Clap;
use std::time::Instant;
use alga::general::Operator;
use swag::reactive::Reactive;
use swag::recalc::ReCalc;
use swag::soe::SoE;
use swag::two_stacks::TwoStacks;
use swag::FifoWindow;
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
    Window: FifoWindow<i32, BinOp>,
    BinOp: Operator
{
    let mut window = Window::new();
    let mut force_side_effect = 0;

    for i in 0..opts.window_size {
        window.push((1 + (i % 101)) as i32);
    }

    assert_eq!(window.len(), opts.window_size);

    let start = Instant::now();

    for i in opts.window_size..opts.iterations {
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
        $opts:ident => [$( [$swag:ident, $($function:ident),+] ),*]
    } => {
            {
                $(
                    $(
                        if $opts.swag == $swag::<i32, $function>::name() {
                            if $opts.function == $function::name() {
                                static_core::<$function, $swag<i32, $function>>(&$opts);
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
        opts =>
            [[ReCalc, Sum, Max],
             [TwoStacks, Sum, Max],
             [Reactive, Sum, Max],
             [SoE, Sum]]
    }

    // Should not reach here
    eprintln!("error: unrecognized swag ({}) or function ({})", opts.swag, opts.function);
    std::process::exit(1);
}
