/* 	
	This framework is for designing a custom block device module (Write), 
 	which is mainly referred to the example "Hello World" in
    "spdk\examples\bdev\hello_world\hello_bdev.c".
 
	Steps to : 
    	1. SPDK Initialization
        2. SPDK starts.
            2.1. Open block device.
            2.2. Open I/O channel.
            2.3. Allocate memory for the write buffer.
            2.4. Fill in content.
            2.5. Write the content to block device.
        3. SPDK app stops.
    The following framework ignore most exception handling.
*/
        

struct bdev_context_t {
    // Self defined
    struct spdk_bdev *bdev;
	struct spdk_bdev_desc *bdev_desc;
	struct spdk_io_channel *bdev_io_channel;
	char *buff;
	uint32_t buff_size;
	char *bdev_name;
	struct spdk_bdev_io_wait_entry bdev_io_wait;
}


static void bdev_write(void *arg){
    struct bdev_context_t *bdev_context = arg;
    int rc = 0;
    rc = spdk_bdev_write(bdev_context->bdev_desc, bdev_context->bdev_io_channel,
             bdev_context->buff, 0, bdev_context->buff_size, write_complete,
             bdev_context); 
    if (rc == -ENOMEM) {
        SPDK_NOTICELOG("Queueing io\n");
        // In case we cannot perform I/O now, queue I/O.
        bdev_context->bdev_io_wait.bdev = bdev_context->bdev;
        bdev_context->bdev_io_wait.cb_fn = bdev_write;
        bdev_context->bdev_io_wait.cb_arg = bdev_context;
        spdk_bdev_queue_io_wait(bdev_context->bdev, 
                                bdev_context->bdev_io_channel,
                                &bdev_context->bdev_io_wait);
    } else if (rc) {
		SPDK_ERRLOG("%s error while writing to bdev: %d\n", spdk_strerror(-rc), rc);
		spdk_put_io_channel(bdev_context->bdev_io_channel);
		spdk_bdev_close(bdev_context->bdev_desc);
		spdk_app_stop(-1);
	}
}


static void bdev_start(void *arg1){
    struct bdev_context_t *bdev_context = arg1;
    bdev_context->bdev = NULL;
    bdev_context->bdev_desc = NULL;
    // 2.1. Open block device.
    spdk_bdev_open_ext(bdev_context->bdev_name, true, spdk_bdev_event_cb, NULL, 
                       &bdev_context->bdev_desc);
    	// spdk_bdev_event_cb is notification callback to be called 
    	// when the bdev triggers asynchronous event such as bdev removal.
    bdev_context->bdev = spdk_bdev_desc_get_bdev(bdev_context->bdev_desc);

    // 2.2. Open I/O channel.
	bdev_context->bdev_io_channel = spdk_bdev_get_io_channel(bdev_context->bdev_desc);

    // 2.3. Allocate memory for the write buffer.
    bdev_context->buff_size = spdk_bdev_get_block_size(bdev_context->bdev) *
				   spdk_bdev_get_write_unit_size(bdev_context->bdev);
	buf_align = spdk_bdev_get_buf_align(bdev_context->bdev);
	bdev_context->buff = spdk_dma_zmalloc(bdev_context->buff_size, buf_align, NULL);

    // 2.4. Initialize the write buffer with the string "Hello World!".
    snprintf(bdev_context->buff, bdev_context->buff_size, "%s", "Hello World!\n");

    // 2.5. Write the content to block device.
	bdev_write(bdev_context);
}


int main(int argc, char** argv){
	struct spdk_app_opts opts = {};
    struct bdev_context_t bdev_context = {};
    // 1. SPDK Initialization
    spdk_app_opts_init(&opts, sizeof(opts)); 
    spdk_app_parse_args(...);	// Parse argument.

    // 2. SPDK starts.
    spdk_app_start(&opt, bdev_start, &bdev_context); // Call function bdev_start.

    // 3. SPDK app stops.
    spdk_dma_free(bdev_context.buff); // Free up memory.
    spdk_app_fini(); // Termination
    return 0;
}
