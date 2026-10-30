/* The page owns each decoded texture once; repeated URLs borrow that owner. */
#pragma once
#include "page.h"
#include "image.h"

bool page_image_accept(Page *page, int index, DecodedImage *image, size_t budget);
void page_images_evict(Page *page, float top, float height);
void page_images_dispose(Page *page);
int page_image_shared(Page *page, int index);  /* ready owner, or -1 */
