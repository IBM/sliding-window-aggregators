/// This is a test-suite for testing if compilation fails or succeeds
#[test]
fn trybuild() {
    let t = trybuild::TestCases::new();
    t.compile_fail("tests/trybuild_*.fail");
    t.pass("tests/trybuild_*.pass");
}
