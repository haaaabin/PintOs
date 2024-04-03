/* file.c: Implementation of memory backed file object (mmaped object). */
/* file.c: memory baked file 객체 구현 (mmaped object).*/

#include "vm/vm.h"
#include "threads/mmu.h"
#include "include/lib/user/syscall.h"
#include "userprog/process.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
/* file vm 초기화*/
void
vm_file_init (void) {
}

/* Initialize the file backed page */
/* file baked page 초기화*/
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
/* 파일로 부터 contents를 읽어서 page를 Swap-In 해라*/
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
/* 파일에 contents를 다시 작성하여 page를 Swap-Out하라 */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
/* file backed page를 파괴하라. page는 호출자에 의해서 해제 될 것이다.*/
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {

	struct file *re_file = file_reopen(file);
	void *tmpaddr = addr;	//addr 주소의 경우 iter를 돌면서 PGSIZE만큼 변경되기 때문에 임시로 저장해둔다.

	// length ->읽어들일 데이터의 길이 , file_length(file)-> 파일의 길이
	int total_page_num = length / PGSIZE + (length % PGSIZE != 0);	//총 페이지 수

	uint32_t read_bytes = file_length(re_file)> length ? length : file_length(re_file);	// 둘 중에 작은 것을 반환
	uint32_t zero_bytes = PGSIZE - (read_bytes % PGSIZE);	// 0으로 채울 바이트 수

	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (addr) == 0);
	ASSERT (offset % PGSIZE == 0);

	//mmap을 하는 동안 만약 외부에서 해당 파일을 close()하는 불상사를 예외처리하기 위함
	if (re_file == NULL) {
		return MAP_FAILED;
	}
	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		/* 이 페이지를 채우는 방법을 계산하세요.
		   FILE에서 PAGE_READ_BYTES 바이트를 읽고
		   최종 PAGE_ZERO_BYTES 바이트를 0으로 합니다. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		/* lazy_load_segment에 정보를 전달하도록 aux를 설정합니다.*/

		struct lazy_load_arg *lazy_load_arg = (struct lazy_load_arg *)malloc(sizeof(struct lazy_load_arg));
		lazy_load_arg->file = re_file;
		lazy_load_arg->ofs = offset;
		lazy_load_arg->read_bytes = page_read_bytes;
		lazy_load_arg->zero_bytes = page_zero_bytes;

		if (!vm_alloc_page_with_initializer (VM_FILE, addr,
					writable, lazy_load_segment, lazy_load_arg))
			return NULL;

		struct page *page = spt_find_page(&thread_current()->spt, addr);
		page->mapped_page_num = total_page_num;
		
		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}
	return tmpaddr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct thread *curr = thread_current();
	struct page *page = spt_find_page(&curr->spt, addr);
	int page_num = page->mapped_page_num;
	for (int i = 0; i < page_num; i++) {
		if (page == NULL) {
			return;
		}
		struct lazy_load_arg *page_aux = page->uninit.aux;
		if(pml4_is_dirty(curr->pml4, page->va)){
			file_write_at(page_aux->file, addr, page_aux->read_bytes, page_aux->ofs);
			pml4_set_dirty(curr->pml4, page->va, false);
		}
		else{
			pml4_clear_page(curr->pml4, page->va);
			//destroy(page);
			addr += PGSIZE;
			page = spt_find_page(&curr->spt, addr);
			// addr을 다음 주소로 변경
		}
		
	}
}