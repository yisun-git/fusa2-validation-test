{
	.suit_id = 367,
	.case_id = 24415,
	.name = "32 Bit Paging support",
	.fn = paging_rqmid_24415_32_bit_paging_support
},
{
	.suit_id = 367,
	.case_id = 32355,
	.name = "32 Bit Paging 4M page support",
	.fn = paging_rqmid_32355_32_bit_paging_4M_page_support
},
{
	.suit_id = 367,
	.case_id = 28958,
	.name = "PSE support",
	.fn = paging_rqmid_28958_PSE_support
},
{
	.suit_id = 367,
	.case_id = 26999,
	.name = "PAE 4K support",
	.fn = paging_rqmid_26999_PAE_4K_support
},
{
	.suit_id = 367,
	.case_id = 32327,
	.name = "PAE 2M support",
	.fn = paging_rqmid_32327_PAE_2M_support
},

{
	.suit_id = 365,
	.case_id = 37209,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR0.PG",
	.fn = paging_rqmid_37209_changes_cr0_pg_001
},
{
	.suit_id = 365,
	.case_id = 37208,
	.name = "Invalidate Paging Structure Cache when vCPU changes CR4.PAE",
	.fn = paging_rqmid_37208_changes_cr4_pae_001
},
{
	.suit_id = 365,
	.case_id = 29529,
	.name = "Invalidate TLB when vCPU changes CR4.PAE",
	.fn = paging_rqmid_29529_cr4_pae_invalidate_tlb
},
{
	.suit_id = 365,
	.case_id = 37210,
	.name = "Invalidate TLB when vCPU changes CR0.PG",
	.fn = paging_rqmid_37210_changes_cr0_pg_001
},

{
	.suit_id = 376,
	.case_id = 47015,
	.name = "Invalidate TLB when guest page level protection 001",
	.fn = paging_rqmid_47015_page_level_protection_inv_tlb
},

{
	.suit_id = 376,
	.case_id = 47016,
	.name = "Invalidate paging structure cache when guest page level protection 001",
	.fn = paging_rqmid_47016_page_level_protection_inv_psc
},
