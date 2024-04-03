/* file.c: Implementation of memory backed file object (mmaped object). */
/* file.c: memory baked file 객체 구현 (mmaped object).*/

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "vm/file.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
void do_munmap (void *addr);
void* do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset);

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
	file_page->aux = page->uninit.aux;

	// struct lazy_load_arg *lazy_load_arg = (struct lazy_load_arg *)page->uninit.aux;
	// file_page->file = lazy_load_arg->file;
	// file_page->ofs = lazy_load_arg->ofs;
	// file_page->read_bytes = lazy_load_arg->read_bytes;
	// file_page->zero_bytes = lazy_load_arg->zero_bytes;
	return true;
}

/* Swap in the page by read contents from the file. */
/* 파일로 부터 contents를 읽어서 page를 Swap-In 해라*/
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;

	if(page == NULL){
		return false;
	}

	//페이지를 로드하기 위한 aux 저장
	struct lazy_load_arg * aux = (struct lazy_load_arg *)page->uninit.aux;
	struct file *file = aux->file;
	off_t offset = aux->ofs;
	size_t page_read_bytes = aux->read_bytes;
	size_t page_zero_bytes = PGSIZE - page_read_bytes;
	
	//파일에서 읽기 시작 위치를 설정
	file_seek(file,offset);

	//파일에서 페이지의 내용을 읽어와 메모리에 로드
	if(file_read(file,kva,page_read_bytes) != (int)page_read_bytes){
		return false;
	}

	//페이지의 남은 부분을 0으로 초기화
	memset(kva + page_read_bytes, 0, page_zero_bytes);

	return true;
}

/* Swap out the page by writeback contents to the file. */
/* 파일에 contents를 다시 작성하여 page를 Swap-Out하라 */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;

	if(page ==NULL){
		return false;
	}
	//page가 수정되었다면 file에 수정사항을 기록하면서 swap out 시킨다.
	//dirty check
	file_backed_destroy(page);	
}

/* Destory the file backed page. PAGE will be freed by the caller. */
/* file backed page를 파괴하라. page는 호출자에 의해서 해제 될 것이다.*/
static void
file_backed_destroy (struct page *page) {
	
	struct thread *t = thread_current();
	struct file_page *file_page UNUSED = &page->file;
	struct lazy_load_arg *file_aux = (struct lazy_load_arg *)file_page->aux;
	struct thread *t = thread_current();

	if(pml4_is_dirty(t->pml4, page->va)){			
		file_write_at(file_aux->file, page->va, file_aux->read_bytes, file_aux->ofs);
		pml4_set_dirty(t->pml4, page->va, 0);
	}
	pml4_clear_page(t->pml4, page->va);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	
	struct file *re_file = file_reopen(file);
	void *start_addr = addr; // 매핑 성공 시 파일이 매핑된 가상 주소 반환하는 데 사용

	size_t read_bytes = file_length(re_file) < length ? file_length(re_file) : length;
	size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;

	ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT(pg_ofs(addr) == 0);	  // upage가 페이지 정렬되어 있는지 확인
	ASSERT(offset % PGSIZE == 0); // ofs가 페이지 정렬되어 있는지 확인

	while (read_bytes > 0 || zero_bytes > 0)
	{
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct lazy_load_arg *lazy_load_arg = (struct lazy_load_arg *)malloc(sizeof(struct lazy_load_arg));
		lazy_load_arg->file = re_file;
		lazy_load_arg->ofs = offset;
		lazy_load_arg->read_bytes = page_read_bytes;
		lazy_load_arg->zero_bytes = page_zero_bytes;

		// vm_alloc_page_with_initializer를 호출하여 대기 중인 객체를 생성합니다.
		if (!vm_alloc_page_with_initializer(VM_FILE, addr,
											writable, lazy_load_segment, lazy_load_arg))
			return NULL;

		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}

	return start_addr;
}

/* Do the munmap */
/*연결된 물리프레임과의 연결을 끊어준다.*/
void
do_munmap (void *addr) {
	while(true){
		struct thread *curr = thread_current();
		struct page *find_page = spt_find_page(&curr->spt, addr);
		
		if (find_page == NULL) {
    		return NULL;
    	}

		struct lazy_load_arg* container = (struct lazy_load_arg*)find_page->uninit.aux;
		find_page->file.aux = container;

		file_backed_destroy(find_page);
		addr += PGSIZE;
	}
}

// void
// do_munmap (void *addr) {

// 	while(true){
// 		struct thread *curr = thread_current();
// 		struct page *find_page = spt_find_page(&curr->spt, addr);
		
// 		if (find_page == NULL) {
//     		return NULL;
//     	}

// 		struct file_page *file_page = &find_page->file;
// 		struct lazy_load_arg* container = (struct lazy_load_arg*)file_page->aux;
	
// 		if (pml4_is_dirty(curr->pml4, find_page->va)){
// 			// 물리 프레임에 변경된 데이터를 다시 디스크 파일에 업데이트 buffer에 있는 데이터를 size만큼, file의 file_ofs부터 써준다.
// 			file_write_at(container->file, addr, container->read_bytes, container->ofs);
// 			pml4_set_dirty(curr->pml4, find_page->va,0);
// 		} 

// 		pml4_clear_page(curr->pml4, find_page->va); 
		
// 		addr += PGSIZE;
// 	}
// }