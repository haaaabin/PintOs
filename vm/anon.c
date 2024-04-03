/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */
/*anon.c : file과 mapping이 되지 않은 익명 페이지 구현*/

#include "vm/vm.h"
#include "devices/disk.h"
#include "include/vm/anon.h"
#define SECTORS_PER_PAGE = PGSIZE / DISK_SECTOR_SIZE; // 8 = 4096 / 512
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

/* Initialize the data for anonymous pages */
/*익명 페이지 초기화*/
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	/*swap_disk 설정*/
	swap_disk = disk_get (1, 1);	//disk.c CHAN_NO로 번호가 매겨진 채널 내의 DEV_NO 번호의 디스크를 반환한다.

	//SECTORS_PER_PAGE = PGSIZE / DISK_SECTOR_SIZE; 8 = 4096 / 512
	swap_size = disk_size(swap_disk)/SECTORS_PER_PAGE; // 디스크 전체 크기를 몇 개의 페이지로 이루어져있는지 계산한다.
	// 디스크 섹터는 하드 디스크 내 정보를 저장하는 단위로, 자체적으로 주소를 갖는 storage의 단위다. 
	// 즉, 한 디스크 당 몇 개의 섹터가 들어가는지를 나눈 값을 swap_size로 지칭한다. 
	// 즉, 해당 swap_disk를 swap할 때 필요한 섹터 수가 결국 swap_size.

	//swap size 크기만큼 swap_table을 비트맵으로 생성
	swap_table = bitmap_create(swap_size); // each bit = swap slot for a frame	성공하면 true 실패하면 false 반환
}

/* Initialize the file mapping */
/*파일 매핑 초기화*/
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
/*Swap disk로 부터 contents를 읽어서 페이지를 Swap-in 하라*/
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
/*Swap disk에 contents를 기록하여 페이지를 Swap-Out 하라*/

static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
/*익명 페이지를 파괴하라. 페이지는 호출자에 의하여 해제된다 */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
