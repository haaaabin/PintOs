/* uninit.c: Implementation of uninitialized page.
			 초기화되지 않은 페이지 구현
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * 
 * 모든 페이지는 초기화되지 않은 페이지로 태어납니다.
 * 첫 번째 페이지 오류가 발생하면 핸들러 체인이 unint_initialize(page->operations.swap_in)를 호출합니다.
 * uninit_initialize 함수는 페이지 객체(anon,file,page_cache)로 변환하고 
 * vm_alloc_page_with_initializer 함수에서 전달된 초기화 콜백을 호출합니다.
 * */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,
	.swap_out = NULL,
	.destroy = uninit_destroy,
	.type = VM_UNINIT,
};

/* DO NOT MODIFY this function */
/* uninit_page를 초기화합니다. */
void
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,	//페이지 초기화를 수행할 함수 포인터 설정
		}
	};
}

/* Initalize the page on first fault 
   첫 번째 오류 시 페이지 초기화 */
/*프로세스가 처음 만들어진(UNINIT)페이지에 처음으로 접근할 때 page fault가 발생한다. 
그러면 page fault handler는 해당 페이지를 디스크에서 프레임으로 swap-in하는데, 
UNINIT type일 때의 swap_in 함수가 바로 이 함수이다. 즉, UNINIT 페이지 멤버를 초기화해줌으로써 
페이지 타입을 인자로 주어진 타입(ANON, FILE, PAGE_CACHE)로 변환시켜준다. 
여기서 만약 segment도 load되지 않은 상태라면 lazy load segment도 진행한다.*/

static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit;

	/* Fetch first, page_initialize may overwrite the values 
	   먼저 가져오기, page_initalize는 값을 덮어쓸 수 있습니다.*/
	/* 데이터나 변수를 초기화하기 전에 그 값을 가져와서 이미 값이 존재할 경우에 
	   대비하여 초기화하는 것이 중요하다. */
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. 
	   TODO: 이 함수를 수정해야 할 수도 있습니다. */
	return uninit->page_initializer (page, uninit->type, kva) &&
		(init ? init (page, aux) : true);
}

/* Free the resources hold by uninit_page. Although most of pages are transmuted
 * to other page objects, it is possible to have uninit pages when the process
 * exit, which are never referenced during the execution.
 * PAGE will be freed by the caller. 
 * 
 * uninit_page가 보유한 리소스를 해제합니다. 대부분의 페이지는 다른 페이지 객체로 변환되지만
 * 프로세스가 종료될 때 실행 중에 참조되지 않는 uninit 페이지가 있을 수도 있습니다.
 * 호출자에 의해 페이지가 해제됩니다.*/
static void
uninit_destroy (struct page *page) {
	struct uninit_page *uninit UNUSED = &page->uninit;
	/* TODO: Fill this function.
	 * TODO: If you don't have anything to do, just return. 
	   이 함수를 채우세요. 할 일이 없으면 그냥 돌아가세요. */
}
