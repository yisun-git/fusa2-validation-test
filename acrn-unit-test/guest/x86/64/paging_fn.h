// suite 356
#ifdef IN_NON_SAFETY_VM
{
	.suit_id = 356,
	.case_id = 32265,
	.name = "IA32 EFER.NXE state following INIT",
	.fn = paging_rqmid_32265_ia32_efer_nxe_init
},
{
	.suit_id = 356,
	.case_id = 32270,
	.name = "EFLAGS.AC state following INIT",
	.fn = paging_rqmid_32270_eflags_ac_init
},
{
	.suit_id = 356,
	.case_id = 32272,
	.name = "CR4.SMEP state following INIT",
	.fn = paging_rqmid_32272_cr4_smep_init
},
{
	.suit_id = 356,
	.case_id = 32274,
	.name = "CR4.SMAP state following INIT",
	.fn = paging_rqmid_32274_cr4_smap_init
},
{
	.suit_id = 356,
	.case_id = 32276,
	.name = "CR4.PSE state following INIT",
	.fn = paging_rqmid_32276_cr4_pse_init
},
{
	.suit_id = 356,
	.case_id = 32278,
	.name = "CR4.PKE state following INIT",
	.fn = paging_rqmid_32278_cr4_pke_init
},
{
	.suit_id = 356,
	.case_id = 32280,
	.name = "CR4.PGE state following INIT",
	.fn = paging_rqmid_32280_cr4_pge_init
},
{
	.suit_id = 356,
	.case_id = 32297,
	.name = "CR4.PAE state following INIT",
	.fn = paging_rqmid_32297_cr4_pae_init
},
{
	.suit_id = 356,
	.case_id = 32290,
	.name = "CR4.PCIDE state following INIT",
	.fn = paging_rqmid_32290_cr4_pcide_init
},
{
	.suit_id = 356,
	.case_id = 32304,
	.name = "CR0.WP state following INIT",
	.fn = paging_rqmid_32304_cr0_wp_init
},
{
	.suit_id = 356,
	.case_id = 32306,
	.name = "CR0.PG state following INIT",
	.fn = paging_rqmid_32306_cr0_pg_init
},
{
	.suit_id = 356,
	.case_id = 32302,
	.name = "CR2 state following INIT",
	.fn = paging_rqmid_32302_cr2_init
},
{
	.suit_id = 356,
	.case_id = 32300,
	.name = "CR3 state following INIT",
	.fn = paging_rqmid_32300_cr3_init
},
#endif
{
	.suit_id = 356,
	.case_id = 32264,
	.name = "IA32 EFER.NXE state following START UP",
	.fn = paging_rqmid_32264_ia32_efer_nxe_start_up
},
{
	.suit_id = 356,
	.case_id = 32269,
	.name = "EFLAGS.AC state following START UP",
	.fn = paging_rqmid_32269_eflags_ac_start_up
},
{
	.suit_id = 356,
	.case_id = 32271,
	.name = "CR4.SMAP state following START UP",
	.fn = paging_rqmid_32271_cr4_smap_start_up
},
{
	.suit_id = 356,
	.case_id = 32273,
	.name = "CR4.SMEP state following START UP",
	.fn = paging_rqmid_32273_cr4_smep_start_up
},
{
	.suit_id = 356,
	.case_id = 32275,
	.name = "CR4.PSE state following START UP",
	.fn = paging_rqmid_32275_cr4_pse_start_up
},
{
	.suit_id = 356,
	.case_id = 32277,
	.name = "CR4.PKE state following START UP",
	.fn = paging_rqmid_32277_cr4_pke_start_up
},
{
	.suit_id = 356,
	.case_id = 32279,
	.name = "CR4.PGE state following START UP",
	.fn = paging_rqmid_32279_cr4_pge_start_up
},
{
	.suit_id = 356,
	.case_id = 32285,
	.name = "CR4.PCIDE state following START UP",
	.fn = paging_rqmid_32285_cr4_pcide_start_up
},
{
	.suit_id = 356,
	.case_id = 32295,
	.name = "CR4.PAE state following START UP",
	.fn = paging_rqmid_32295_cr4_pae_start_up
},
{
	.suit_id = 356,
	.case_id = 32298,
	.name = "CR3 state following START UP",
	.fn = paging_rqmid_32298_cr3_start_up
},
{
	.suit_id = 356,
	.case_id = 32301,
	.name = "CR2 state following START UP",
	.fn = paging_rqmid_32301_cr2_start_up
},
{
	.suit_id = 356,
	.case_id = 32303,
	.name = "CR0.WP state following START UP",
	.fn = paging_rqmid_32303_cr0_wp_start_up
},
{
	.suit_id = 356,
	.case_id = 32305,
	.name = "CR0.PG state following START UP",
	.fn = paging_rqmid_32305_cr0_pg_start_up
},

// suite 365
{
	.suit_id = 365,
	.case_id = 24519,
	.name = "Invalidate TLB when vCPU writes CR3 disable global paging",
	.fn = paging_rqmid_24519_disable_global_paging
},
{
	.suit_id = 365,
	.case_id = 26827,
	.name = "Invalidate TLB when vCPU writes CR3 enable global paging",
	.fn = paging_rqmid_26827_enable_global_paging
},
{
	.suit_id = 365,
	.case_id = 24460,
	.name = "Invalidate TLB when vCPU changes CR4.SMAP",
	.fn = paging_rqmid_24460_cr4_smap_invalidate_tlb
},
{
	.suit_id = 365,
	.case_id = 32328,
	.name = "Invalidate TLB when vCPU changes IA32 EFER.NXE",
	.fn = paging_rqmid_32328_ia32_efer_nxe_tlb
},
{
	.suit_id = 365,
	.case_id = 32329,
	.name = "Invalidate TLB when vCPU changes CR4.SMEP",
	.fn = paging_rqmid_32329_cr4_smep_tlb
},
{
	.suit_id = 365,
	.case_id = 32344,
	.name = "Invalidate TLB when vCPU changes CR4.PSE",
	.fn = paging_rqmid_32344_cr4_pse_tlb
},
{
	.suit_id = 365,
	.case_id = 32406,
	.name = "Invalidate TLB when vCPU changes CR4.PGE",
	.fn = paging_rqmid_32406_cr4_pge_tlb
},
{
	.suit_id = 365,
	.case_id = 32234,
	.name = "Invalidate TLB when vCPU changes CR0.WP",
	.fn = paging_rqmid_32234_cr0_wp_tlb
},
{
	.suit_id = 365,
	.case_id = 32236,
	.name = "Invalidate TLB when vCPU changes CR0.CD",
	.fn = paging_rqmid_32236_cr0_cd_tlb
},
{
	.suit_id = 365,
	.case_id = 24246,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR4.PGE",
	.fn = paging_rqmid_24246_changes_cr4_pge_001
},
{
	.suit_id = 365,
	.case_id = 37207,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR4.SMEP",
	.fn = paging_rqmid_37207_changes_cr4_smep_001
},
{
	.suit_id = 365,
	.case_id = 37206,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR0.WP",
	.fn = paging_rqmid_37206_changes_cr0_wp_001
},
{
	.suit_id = 365,
	.case_id = 37205,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR4.PSE",
	.fn = paging_rqmid_37205_changes_cr4_pse_001
},
{
	.suit_id = 365,
	.case_id = 37204,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR4.SMAP",
	.fn = paging_rqmid_37204_changes_cr4_smap_001
},
{
	.suit_id = 365,
	.case_id = 37202,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR0.CD",
	.fn = paging_rqmid_37202_changes_cr0_cd_001
},
{
	.suit_id = 365,
	.case_id = 37201,
	.name = "Invalidate Paging Structure Cache when vCPU changes IA32_EFER.NXE",
	.fn = paging_rqmid_37201_changes_ia32_efer_nxe_001
},
{
	.suit_id = 365,
	.case_id = 32244,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR3",
	.fn = paging_rqmid_32244_changes_cr3_001
},

// suite 370
{
	.suit_id = 370,
	.case_id = 32350,
	.name = "check error code rsvd bit",
	.fn = paging_rqmid_32350_check_error_code_rsvd_bit
},
{
	.suit_id = 370,
	.case_id = 32351,
	.name = "check error code wr bit",
	.fn = paging_rqmid_32351_check_error_code_wr_bit
},
{
	.suit_id = 370,
	.case_id = 32352,
	.name = "check error code p bit",
	.fn = paging_rqmid_32352_check_error_code_p_bit
},
{
	.suit_id = 370,
	.case_id = 32353,
	.name = "check error code all bit",
	.fn = paging_rqmid_32353_check_error_code_all_bit
},
{
	.suit_id = 370,
	.case_id = 32558,
	.name = "check error code U/S bit",
	.fn = paging_rqmid_32558_check_error_code_us_bit
},
{
	.suit_id = 370,
	.case_id = 32354,
	.name = "check error code I/D bit",
	.fn = paging_rqmid_32354_check_error_code_id_bit
},

// suite 376
{
	.suit_id = 376,
	.case_id = 23896,
	.name = "processor cache and TLB info",
	.fn = paging_rqmid_23896_check_tlb_info
},
{
	.suit_id = 376,
	.case_id = 24522,
	.name = "TLB support",
	.fn = paging_rqmid_24522_tlb_support
},

// suite 377
{
	.suit_id = 377,
	.case_id = 23895,
	.name = "physical address width",
	.fn = paging_rqmid_23895_check_physical_address_width
},
{
	.suit_id = 377,
	.case_id = 32346,
	.name = "linear address width",
	.fn = paging_rqmid_32346_check_linear_address_width
},
{
	.suit_id = 377,
	.case_id = 23918,
	.name = "write protect support",
	.fn = paging_rqmid_23918_write_protect_support
},
{
	.suit_id = 377,
	.case_id = 26017,
	.name = "SMEP support",
	.fn = paging_rqmid_26017_smep_support
},
{
	.suit_id = 377,
	.case_id = 32224,
	.name = "write super address at ring3",
	.fn = paging_rqmid_32224_ring3_access_super_address
},
{
	.suit_id = 377,
	.case_id = 32227,
	.name = "write user address at ring3",
	.fn = paging_rqmid_32227_ring3_access_user_address
},
{
	.suit_id = 377,
	.case_id = 32228,
	.name = "SMAP AC write protect",
	.fn = paging_rqmid_32228_write_protect_support_011
},
{
	.suit_id = 377,
	.case_id = 32229,
	.name = "SMAP AC read-only write protect",
	.fn = paging_rqmid_32229_write_protect_support_010
},
{
	.suit_id = 377,
	.case_id = 32230,
	.name = "WP and SMAP AC read-write",
	.fn = paging_rqmid_32230_write_protect_support_009
},
{
	.suit_id = 377,
	.case_id = 32231,
	.name = "WP and no SMAP read-only",
	.fn = paging_rqmid_32231_write_protect_support_008
},
{
	.suit_id = 377,
	.case_id = 32232,
	.name = "WP and no SMAP read-write",
	.fn = paging_rqmid_32232_write_protect_support_007
},
{
	.suit_id = 377,
	.case_id = 32233,
	.name = "no WP and enable SMAP",
	.fn = paging_rqmid_32233_write_protect_support_006
},
{
	.suit_id = 377,
	.case_id = 32307,
	.name = "no WP and no SMAP",
	.fn = paging_rqmid_32307_write_protect_support_005
},
{
	.suit_id = 377,
	.case_id = 32308,
	.name = "write protect support_004",
	.fn = paging_rqmid_32308_write_protect_support_004
},
{
	.suit_id = 377,
	.case_id = 32309,
	.name = "write protect support_003",
	.fn = paging_rqmid_32309_write_protect_support_003
},
{
	.suit_id = 377,
	.case_id = 32310,
	.name = "write protect support_002",
	.fn = paging_rqmid_32310_write_protect_support_002
},
{
	.suit_id = 377,
	.case_id = 32311,
	.name = "SMEP support_002",
	.fn = paging_rqmid_32311_smep_test_002
},
{
	.suit_id = 377,
	.case_id = 32312,
	.name = "SMAP support_003",
	.fn = paging_rqmid_32312_smap_003
},
{
	.suit_id = 377,
	.case_id = 32326,
	.name = "SMAP support_002",
	.fn = paging_rqmid_32326_smap_002
},
{
	.suit_id = 377,
	.case_id = 25233,
	.name = "SMAP support_001",
	.fn = paging_rqmid_25233_smap_001
},
{
	.suit_id = 377,
	.case_id = 32266,
	.name = "execute disable support_005",
	.fn = paging_rqmid_32266_execute_disable_support_005
},
{
	.suit_id = 377,
	.case_id = 32407,
	.name = "execute disable support_004",
	.fn = paging_rqmid_32407_execute_disable_support_004
},
{
	.suit_id = 377,
	.case_id = 32267,
	.name = "execute disable support_003",
	.fn = paging_rqmid_32267_execute_disable_support_003
},
{
	.suit_id = 377,
	.case_id = 32268,
	.name = "execute disable support_002",
	.fn = paging_rqmid_32268_execute_disable_support_002
},
{
	.suit_id = 377,
	.case_id = 25249,
	.name = "execute disable support_002",
	.fn = paging_rqmid_25249_execute_disable_support_001
},
{
	.suit_id = 377,
	.case_id = 23901,
	.name = "global pages support",
	.fn = paging_rqmid_23901_global_pages_support
},

// suite 378
{
	.suit_id = 378,
	.case_id = 23897,
	.name = "hide processor context identifiers",
	.fn = paging_rqmid_23897_hide_processor_context_identifiers
},
{
	.suit_id = 378,
	.case_id = 23917,
	.name = "Protection keys hide",
	.fn = paging_rqmid_23917_protection_keys_hide
},
{
	.suit_id = 378,
	.case_id = 23912,
	.name = "hide invalidate processor context identifiers",
	.fn = paging_rqmid_23912_hide_invalidate_processor_context_identifiers
},

// suite 367
{
	.suit_id = 367,
	.case_id = 32347,
	.name = "4-level paging 4K page support",
	.fn = paging_rqmid_32347_4level_4K_page_support
},
{
	.suit_id = 367,
	.case_id = 32348,
	.name = "4-level paging 2M page support",
	.fn = paging_rqmid_32348_4level_2M_page_support
},
{
	.suit_id = 367,
	.case_id = 32349,
	.name = "4-level paging 1G page support",
	.fn = paging_rqmid_32349_4level_1G_page_support
},

{
	.suit_id = 613,
	.case_id = 35133,
	.name = "Page Size Extension support on physical platform",
	.fn = paging_rqmid_35133_page_size_extension_support_constraint_AC_001
},
