/* abi_error_paths.c — 错误路径全覆盖: 每个带指针参数的导出符号，传 NULL 必须返回 SOV_ERR_NULL(-4)。
 * 例外: 明确不检查 NULL 的符号（如 sov_nse_free(NULL) 合法）单独标注。 */
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

/* �� 弱符号: 宿主由 ghc 链接时先初始化 RTS (hs 库要求), 否则跳过 */
extern void hs_init(int *argc, char **argv[]) __attribute__((weak));

static void *H;
static void *sym(const char *n) { void *p = dlsym(H, n); if (!p) { fprintf(stderr, "missing %s\n", n); _exit(2); } return p; }

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s lib.so\n", argv[0]); return 1; }
    if (hs_init) { hs_init(NULL, NULL); printf("host_hs_init=1\n"); } else printf("host_hs_init=0\n");
    H = dlopen(argv[1], RTLD_NOW);
    if (!H) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    uint8_t buf[64] = {0}; uint16_t u16 = 0; uint64_t u64 = 0; int32_t i32 = 0; int64_t i64 = 0;
    uint8_t t6[6] = {0}; uint8_t packed = 0; char ch = 0; size_t sz = 0; int64_t m64 = 0;
    uint64_t limbs[6] = {0}; uint64_t out8[8] = {0}; uint8_t u8 = 0;

    /* 全部 NULL 调用；输出 "name=code" */
    printf("trit_add=%d\n", ((int32_t(*)(uint8_t,uint8_t,uint8_t*))sym("sov_trit_add"))(0,0,NULL));
    printf("trit_mul=%d\n", ((int32_t(*)(uint8_t,uint8_t,uint8_t*))sym("sov_trit_mul"))(0,0,NULL));
    printf("trit_sun=%d\n", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_trit_sun"))(0,NULL));
    printf("trit_c3=%d\n", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_trit_c3"))(0,NULL));
    printf("gf9_mul=%d\n", ((int32_t(*)(uint8_t,uint8_t,uint8_t*))sym("sov_gf9_mul"))(0,0,NULL));
    printf("gf9_norm=%d\n", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_gf9_norm"))(0,NULL));
    printf("ring_add=%d\n", ((int32_t(*)(const uint8_t*,const uint8_t*,uint8_t*))sym("sov_ring_add"))(buf,buf,NULL));
    printf("ring_mul=%d\n", ((int32_t(*)(const uint8_t*,const uint8_t*,uint8_t*))sym("sov_ring_mul"))(buf,buf,NULL));
    printf("ring_double=%d\n", ((int32_t(*)(const uint8_t*,uint8_t*))sym("sov_ring_double"))(buf,NULL));
    printf("ring_to_tryte=%d\n", ((int32_t(*)(const uint8_t*,uint16_t*))sym("sov_ring_to_tryte"))(buf,NULL));
    printf("dc_mixedop=%d\n", ((int32_t(*)(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t*,uint8_t*))sym("sov_dc_mixedop"))(0,0,0,0,NULL,NULL));
    printf("dc_translate=%d\n", ((int32_t(*)(uint32_t,uint8_t,uint8_t,uint8_t*,uint8_t*))sym("sov_dc_translate"))(0,0,0,NULL,NULL));
    printf("dc_crt=%d\n", ((int32_t(*)(uint8_t,uint8_t,uint8_t*))sym("sov_dc_crt"))(0,0,NULL));
    printf("dc_sigma=%d\n", ((int32_t(*)(uint8_t,uint8_t,uint8_t*,uint8_t*))sym("sov_dc_sigma"))(0,0,NULL,NULL));
    printf("dc_phase_vec=%d\n", ((int32_t(*)(uint8_t,uint8_t*,uint8_t*))sym("sov_dc_phase_vec"))(0,NULL,NULL));
    printf("dc_rotate_vec=%d\n", ((int32_t(*)(uint8_t,uint8_t,uint8_t,uint8_t*,uint8_t*))sym("sov_dc_rotate_vec"))(0,0,0,NULL,NULL));
    printf("adc64=%d\n", ((int32_t(*)(uint64_t,uint64_t,uint8_t,uint64_t*,uint8_t*))sym("sov_adc64"))(0,0,0,NULL,NULL));
    printf("lcm_step=%d\n", ((int32_t(*)(uint64_t*,uint64_t,uint32_t*))sym("sov_lcm_step"))(limbs,0,NULL));
    printf("dayan=%d\n", ((int32_t(*)(uint32_t,uint32_t,uint32_t*))sym("sov_dayan_multiplier"))(1,2,NULL));
    printf("crt2=%d\n", ((int32_t(*)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*))sym("sov_crt2"))(0,1,0,1,NULL));
    printf("zhonglv_step=%d\n", ((int32_t(*)(int64_t,int64_t,int64_t*))sym("sov_zhonglv_step"))(1,2,NULL));
    printf("zhonglv_closure=%d\n", ((int32_t(*)(uint64_t,uint64_t*))sym("sov_zhonglv_closure"))(1,NULL));
    printf("sovereign_lcm=%d\n", ((int32_t(*)(uint64_t*))sym("sov_sovereign_lcm"))(NULL));
    printf("width_cap_t=%d\n", ((int32_t(*)(uint8_t,uint64_t*))sym("sov_width_capacity_trits"))(1,NULL));
    printf("width_cap_b=%d\n", ((int32_t(*)(uint8_t,uint64_t*))sym("sov_width_capacity_bits"))(1,NULL));
    printf("width_t4b=%d\n", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_width_trits_for_bits"))(1,NULL));
    printf("width_b4t=%d\n", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_width_bits_for_trits"))(1,NULL));
    printf("wide_rho=%d\n", ((int32_t(*)(uint64_t,uint64_t*))sym("sov_wide_rho"))(0,NULL));
    printf("wide_pi=%d\n", ((int32_t(*)(uint64_t*))sym("sov_wide_const_pi"))(NULL));
    printf("wide_sqrt3=%d\n", ((int32_t(*)(uint64_t*))sym("sov_wide_const_sqrt3"))(NULL));
    printf("soliton_sin=%d\n", ((int32_t(*)(int64_t,int64_t*))sym("sov_soliton_sin_q32"))(0,NULL));
    printf("soliton_rho=%d\n", ((int32_t(*)(uint32_t,int32_t*))sym("sov_soliton_rho_q16"))(0,NULL));
    printf("soliton_phase=%d\n", ((int32_t(*)(int32_t,uint8_t*))sym("sov_soliton_phase"))(0,NULL));
    printf("u384_add=%d\n", ((int32_t(*)(const uint64_t*,const uint64_t*,uint64_t*,uint8_t*))sym("sov_u384_add"))(limbs,limbs,NULL,NULL));
    printf("u384_mul=%d\n", ((int32_t(*)(const uint64_t*,uint64_t,uint64_t*,uint64_t*))sym("sov_u384_mul_u64"))(limbs,2,NULL,NULL));
    printf("u384_shr=%d\n", ((int32_t(*)(const uint64_t*,uint64_t*))sym("sov_u384_shr16"))(limbs,NULL));
    printf("chern_inject=%d\n", ((int32_t(*)(int64_t*,int64_t*))sym("sov_chern_inject"))(NULL,NULL));
    printf("chern_detect=%d\n", ((int32_t(*)(uint8_t,int64_t*,int64_t*))sym("sov_chern_detect"))(0,NULL,NULL));
    printf("eis_mul=%d\n", ((int32_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t*,int64_t*))sym("sov_eis_mul"))(1,2,3,4,NULL,NULL));
    printf("eis_norm=%d\n", ((int32_t(*)(int64_t,int64_t,int64_t*))sym("sov_eis_norm"))(1,2,NULL));
    printf("holographic=%d\n", ((int32_t(*)(uint32_t,uint32_t,int32_t*))sym("sov_holographic"))(0,1,NULL));
    printf("digital_root=%d\n", ((int32_t(*)(uint64_t,uint8_t*))sym("sov_digital_root"))(1,NULL));
    printf("digital_root_mod9=%d\n", ((int32_t(*)(uint64_t,uint8_t*))sym("sov_digital_root_mod9"))(1,NULL));
    printf("doz_digit=%d\n", ((int32_t(*)(uint8_t,char*))sym("sov_doz_digit"))(0,NULL));
    printf("format_doz=%d\n", ((int32_t(*)(uint64_t,char*,size_t,size_t*))sym("sov_format_doz"))(0,NULL,0,NULL));
    printf("parse_doz=%d\n", ((int32_t(*)(const char*,size_t,uint64_t*))sym("sov_parse_doz"))(NULL,0,NULL));
    printf("tryte_value=%d\n", ((int32_t(*)(const uint8_t*,uint16_t*))sym("sov_tryte_value"))(t6,NULL));
    printf("tryte_unpack=%d\n", ((int32_t(*)(uint16_t,uint8_t*))sym("sov_tryte_unpack"))(0,NULL));
    printf("pack5=%d\n", ((int32_t(*)(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t*))sym("sov_trit_pack5"))(0,0,0,0,0,NULL));
    printf("unpack5=%d\n", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_trit_unpack5"))(0,NULL));
    printf("t6_index=%d\n", ((int32_t(*)(uint32_t,uint32_t,uint16_t*))sym("sov_t6_index"))(0,0,NULL));
    printf("sunyi_boundary=%d\n", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_sunyi_boundary"))(0,NULL));
    printf("standing_node=%d\n", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_is_standing_node"))(0,NULL));
    printf("chiral=%d\n", ((int32_t(*)(int32_t,uint8_t*))sym("sov_chiral_collapse"))(0,NULL));
    printf("chern_compute=%d\n", ((int32_t(*)(const uint8_t*,size_t,int64_t*,int64_t*))sym("sov_chern_compute"))(NULL,0,NULL,NULL));
    printf("chern_inject_monopole=%d\n", ((int32_t(*)(uint8_t*,size_t))sym("sov_chern_inject_monopole"))(NULL,0));
    /* NSE 句柄: NULL 句柄 */
    printf("nse_new=%d\n", ((int32_t(*)(uint8_t,void**))sym("sov_nse_new"))(0,NULL));
    printf("nse_step=%d\n", ((int32_t(*)(void*))sym("sov_nse_step"))(NULL));
    printf("nse_cell=%d\n", ((int32_t(*)(void*,uint16_t,uint16_t,uint8_t*))sym("sov_nse_cell"))(NULL,0,0,&u8));
    printf("nse_step_count=%d\n", ((int32_t(*)(void*,uint32_t*))sym("sov_nse_step_count"))(NULL,NULL));
    printf("nse_density=%d\n", ((int32_t(*)(void*,int32_t*))sym("sov_nse_density_q16"))(NULL,NULL));
    printf("nse_component=%d\n", ((int32_t(*)(const void*,uint16_t,uint8_t,uint8_t*))sym("sov_nse_component"))(NULL,0,0,&u8));
    printf("nse_free=%d\n", ((int32_t(*)(void*))sym("sov_nse_free"))(NULL));   /* free(NULL) 合法 → 0 */
    dlclose(H);
    return 0;
}
