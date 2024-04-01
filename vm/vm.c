/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

// 프레임 구조체를 관리하는 frame_table
//-> 어떠한 함수에서는 이를 초기화시켜야 할 것.
struct list frame_table;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes.
 * 각 서브시스템의 초기화 코드를 호출하여 가상 메모리 서브시스템을 초기화합니다.
 */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* 위에 줄을 수정하지 마세요. */
	/* TODO: Your code goes here. */
	/* 할일 : 여기에 코드를 작성하세요. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
/* 페이지 유형을 가져옵니다.
 *  이 함수는 페이지가 초기화된 후 유형을 알고 싶을 때 유용합니다.
 * 이 함수는 현재 완전히 구현되어 있습니다. */
enum vm_type
page_get_type(struct page *page)
{
	int ty = VM_TYPE(page->operations->type);
	switch (ty)
	{
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
/* 이니셜라이저를 사용해서 보류 중인 페이지 객체를 만듭니다.
 * 페이지를 만들고 싶다면 직접 만들지 말고 이 함수나 `vm_alloc_page`를 통해 만드세요.
 */

bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Check wheter the upage is already occupied or not. */
	/* 'upage'가 이미 사용중인지 여부를 확인한다.*/
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		/* 할일: 페이지를 만들고 VM 유형에 따라 이니셜라이저를 가져와서
		 * 할일: uninit_new를 호출하여 "uninit" 페이지 구조체를 만듭니다.
		 * 할일: uninit_new를 호출한 후에 필드를 수정해야 합니다.
		 */

		struct page *page = (struct page *)malloc(sizeof(struct page));
		if (page == NULL)
		{
			return NULL;
		}

		typedef bool (*page_initializer)(struct page *, enum vm_type, void *kva);
		page_initializer new_initializer = NULL;
		switch (VM_TYPE(type))
		{
		case VM_ANON:
			new_initializer = anon_initializer;
			break;
		case VM_FILE:
			new_initializer = file_backed_initializer;
			break;
		}

		uninit_new(page, upage, init, type, aux, new_initializer);

		page->writable = writable;
		/* TODO: Insert the page into the spt. */
		/* 할일: 페이지를 spt에 삽입합니다. */
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
/* spt로부터 VA를 찾고 페이지를 반환합니다. 에러인 경우 NULL을 반환합니다. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	struct page *page = malloc(sizeof(struct page));
	struct hash_elem *e;
	if (page == NULL)
	{
		return NULL;
	}

	/* 할일: 이 함수를 채워주세요. */
	page->va = pg_round_down(va);
	e = hash_find(&spt->hash_table, &page->hash_elem);

	if (e != NULL)
	{
		struct page *found_page = hash_entry(e, struct page, hash_elem);
		free(page);
		return found_page;
	}
	else
	{
		free(page);
		return NULL;
	}
}

/* Insert PAGE into spt with validation. */
/* 페이지를 유효성 검사를 거쳐 spt에 삽입합니다. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED, struct page *page UNUSED)
{
	int succ = false;

	/* 할일: 이 함수를 채워주세요. */
	if (is_user_vaddr(page->va))
	{
		if (spt_find_page(spt, page->va) == NULL)
		{
			hash_insert(&spt->hash_table, &page->hash_elem);
			succ = true;
		}
	}
	return succ;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
/* 페이지를 교체할 프레임을 가져옵니다. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */
	/* 할일: 교체 정책은 여러분이 정하세요. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
/* 페이지를 교체하고 해당 프레임을 반환합니다.
 * 에러인 경우 NULL을 반환합니다. */

static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */
	/* 할일: 희생자를 교체하고 교체된 프레임을 반환합니다. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
/* palloc() 및 프레임을 가져옵니다. 사용 가능한 페이지가 없는 경우 페이지를
 * 교체하고 반환합니다. 이 함수는 항상 유효한 주소를 반환합니다. 즉, 사용자 풀
 * 메모리가 가득 찬 경우 이 함수는 사용 가능한 메모리 공간을 얻기 위해 프레임을
 * 교체합니다. */

static struct frame *
vm_get_frame(void)
{
	struct frame *frame = malloc(sizeof(struct frame));
	if (frame == NULL)
	{
		PANIC("todo");
	}
	/* 할일: 이 함수를 채워주세요. */
	/*kernel 가상주소 초기화*/
	frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);

	if (frame->kva == NULL)
	{
		free(frame);
		PANIC("todo");
		// return vm_evict_frame();
	}

	frame->page = NULL;

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);

	return frame;
}

/* 스택을 확장합니다. */
static void
vm_stack_growth(void *va UNUSED)
{
}

/* Handle the fault on write_protected page */
/* 쓰기 보호된 페이지에 대한 처리 */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
/* 성공 시 true를 반환합니다. */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
						 bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	/* 할일: 페이지 폴트를 검증하세요. */
	/* 할일: 여기에 코드를 작성하세요. */
	// printf("vm_try_handle_fault %p \n", addr);

	if (addr == NULL || (is_kernel_vaddr(addr)))
	{
		return false;
	}

	page = spt_find_page(spt, addr);
	if (page == NULL)
	{
		return false;
	}

	if (not_present)
	{
		bool success = vm_do_claim_page(page);
		if (!success)
		{
			return false;
		}
	}

	return true;
}

/* Free the page.
/* DO NOT MODIFY THIS FUNCTION. */
/* 페이지를 해제합니다. */
/* 이 함수를 수정하지 마세요. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
/* VA(페이지의 가상 메모리 주소)를 통해 페이지를 얻어온다. */
bool vm_claim_page(void *va UNUSED)
{
	struct page *page = NULL;
	/* 할일: 이 함수를 채워주세요. */
	/* 물리 프레임과 연결을 할 페이지를 SPT를 통해서 찾아준다.*/
	page = spt_find_page(&thread_current()->spt, va);

	if (page == NULL)
		return false;
	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
/* 페이지를 청구하고 mmu를 설정합니다.
   실질적으로 frame과 인자로 받은 page를 연결해주는 역할 수행.
   -> 해당 페이지가 이미 어떠한 물리 주소(kva)와 미리 연결이 되어 있는지 확인해야 한다.
   -> 이후 미리 연결된 kva가 없을 경우, 해당 va를 kva에 set해준다. */
static bool
vm_do_claim_page(struct page *page)
{
	struct frame *frame = vm_get_frame();
	if (frame == NULL)
	{
		return false;
	}

	/* Set links */
	/* 링크 설정 */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* 할 일: 페이지 테이블 항목을 삽입하여 페이지의 VA를 프레임의 PA에 매핑합니다. */
	if (pml4_get_page(thread_current()->pml4, page->va) == NULL)
	{
		if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, true))
		{
			vm_dealloc_page(page);
			return false;
		}
	}
	/* 해당 페이지를 물리 메모리에 올려준다.*/
	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
/* 새 보조 페이지 테이블을 초기화합니다. */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	hash_init(&spt->hash_table, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
/* src에서 dst로 보조 페이지 테이블을 복사합니다. */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
	
}

/* Free the resource hold by the supplemental page table */
/* 보조 페이지 테이블에 의해 보유된 리소스를 해제합니다. */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	/* 할일: 스레드에 의해 보유된 모든 보조 페이지 테이블을 파괴하고
	 * 할일: 변경된 모든 내용을 저장소에 기록하세요. */
}
