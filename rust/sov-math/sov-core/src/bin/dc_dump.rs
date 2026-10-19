fn main() {
    for p in sov_core::dc_clock::all_points() {
        println!("DCCRT,{}", sov_core::dc_clock::to_duodec(p));
    }
}
