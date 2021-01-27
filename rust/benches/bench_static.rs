use clap::Clap;
use std::time::Instant;
use alga::general::Operator;
//use swag::reactive::Reactive;
//use swag::recalc::ReCalc;
//use swag::soe::SoE;
use swag::two_stacks::TwoStacks;
use swag::FifoWindow;
use swag::ops::Int;
use swag::ops::Sum;
//use swag::ops::Max;

#[derive(Clap)]
struct Opts {
    // cargo calls us with --bench, so we just need to consume it
    #[clap(long, parse(try_from_str), default_value="true")]
    _bench: String,

    // cargo passes our own name, so we just need to consume it
    _name: String,

    aggregator: String,
    function: String,
    window_size: usize,
    iterations: usize,

    #[clap(long, parse(try_from_str), default_value="false")]
    latency: bool
}

fn static_core<BinOp, Window>(opts: &Opts)
where
    Window: FifoWindow<Int, BinOp>,
    BinOp: Operator
{
    let mut window = Window::new();
    let mut force_side_effect = Int(0);

    for i in 0..opts.window_size {
        window.push(Int((1 + (i % 101)) as i32));
    }

    assert_eq!(window.len(), opts.window_size);

    let start = Instant::now();

    for i in opts.window_size..opts.iterations {
        window.pop();
        window.push(Int((1 + (i % 101)) as i32));
        force_side_effect.0 += window.query().0;
    }

    let duration = start.elapsed();
    println!("core runtime: {:?}", duration);
    println!("{}", force_side_effect.0);
}

fn main() {
    let opts: Opts = Opts::parse();

    println!("agg {}, fun {}, win {}, it {}, lat {}", opts.aggregator, 
                                                      opts.function, 
                                                      opts.window_size, 
                                                      opts.iterations, 
                                                      opts.latency);
    static_core::<Sum, TwoStacks<Int, Sum>>(&opts);
}
