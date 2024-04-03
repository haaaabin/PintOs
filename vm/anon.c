/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */
/*anon.c : file과 mapping이 되지 않은 익명 페이지 구현*/

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"	

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

//8개의 disk sector가 page마다 있는 것이다.
const size_t SECTORS_PER_PAGE = PGSIZE / DISK_SECTOR_SIZE;	// 8 = 4096 / 512

/* Initialize the data for anonymous pages */
/*익명 페이지 초기화*/
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	/*swap_disk 설정*/
	swap_disk = disk_get(1,1);
	size_t swap_size = disk_size(swap_disk) / SECTORS_PER_PAGE;
	
	//모든 bit들을 false로 초기화, 사용되면 bit를 true로 바꾼다.
	swap_table = bitmap_create(swap_size);
}

/* Initialize the file mapping */
/*파일 매핑 초기화*/
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	anon_page->swap_sector = -1;	//-1은 스왑 섹터가 할당되지 않았음
	return true;
}

/* Swap in the page by read contents from the swap disk. */
/*Swap disk로 부터 contents를 읽어서 페이지를 Swap-in 하라*/
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;

	/*
	디스크에서 메모리로 데이터를 읽어서 스왑 디스크에서 익명 페이지로 스왑한다.
	데이터의 위치는 페이지가 스왑 아웃될 때 페이지 구조에 스왑 디스크가 저장되어 있어야 한다는 것이다.
	스왑 테이블을 업데이트해야 한다.
	*/
	int find_slot = anon_page->swap_sector;

	if(bitmap_test(swap_table, find_slot) == false){	//스왑 테이블에 해당 슬롯(섹터)가 있는지 확인
		return false;
	}

	for(int i = 0; i <SECTORS_PER_PAGE; i++){	//디스크로부터 읽어온다.
		disk_read(swap_disk, find_slot *SECTORS_PER_PAGE+ i, kva + DISK_SECTOR_SIZE*i);
	}

	bitmap_set(swap_table,find_slot, false);	//해당 슬롯이 스왑인 되어있다는 표시

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
/*Swap disk에 contents를 기록하여 페이지를 Swap-Out 하라*/

static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;

	//swap table에서 page를 할당받을 수 있는 swap slot 찾기
	//bitmap_scan : 비트맵에서 비트를 검색하여 주어진 범위에서 비트를 찾는다.
	int empty_slot = bitmap_scan(swap_table, 0, 1, false);
	if(empty_slot == BITMAP_ERROR){
		return false;
	}
	
	/*
	한 페이지를 디스크에 써주기 위해 SECTORS_PER_PAGE 개의 섹터에 저장해야 한다.
	이때 디스크에 각 섹터 크기의 DISK_SECTOR_SIZE만큼 써준다.
	*/
	for(int i = 0; i <SECTORS_PER_PAGE; i++){
		disk_write(swap_disk, empty_slot *SECTORS_PER_PAGE + i , page->va + DISK_SECTOR_SIZE * i);
	}

	/*
	swap table의 해당 페이지에 대한 swap slot의 비트를 true로 바꿔주고
	해당 페이지의 PTE에서 present bit를 0으로 바꿔준다.
	이제 프로세스가 이 페이지에 접근하면 page fault가 뜬다.
	*/

	//스왑 테이블에서 해당 스왑 슬롯을 사용 중으로 설정한다.
	bitmap_set(swap_table, empty_slot, true);
	pml4_clear_page(thread_current()->pml4, page->va);

	//페이지에 대한 스왑 인덱스 값을 이 페이지가 저장된 swap slot의 번호로 써준다.
	anon_page->swap_sector = empty_slot;

	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
/*익명 페이지를 파괴하라. 페이지는 호출자에 의하여 해제된다 */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
