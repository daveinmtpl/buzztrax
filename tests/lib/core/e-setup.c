/** $Id: e-setup.c,v 1.2 2004-10-01 13:36:09 waffel Exp $
**/

#include "t-core.h"

//-- globals

GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);

//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  //puts(__FILE__":teardown");
}

//-- tests

/**
* Try to create a bt_machine_iterator with a real setup parameter.
* This case demonstrates, how to create a machine iterator from the setup.
* The returned pointer to the iterator should not be null.
*
*/
START_TEST(test_btsetup_obj1){
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	// machine
	BtSourceMachine *gen1=NULL;
	BtMachine *ref_machine=NULL;
	
	gpointer *iter_ptr;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  gen1 = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(gen1!=NULL, NULL);
	
	/* try to add the machine to the setup (and therewith to the song) */
	bt_setup_add_machine(setup,BT_MACHINE(gen1));
	
	iter_ptr=bt_setup_machine_iterator_new(setup);
	/* the iterator should not be null */
	fail_unless(iter_ptr!=NULL, NULL);
	
	/* the pointer should be point to the gen1 machine */
	ref_machine=bt_setup_machine_iterator_get_machine(iter_ptr);
	fail_unless(ref_machine!=NULL, NULL);
	fail_unless(BT_IS_SOURCE_MACHINE(ref_machine)==TRUE, NULL);
	
	/* the next element of the pointer should be null */
	fail_unless(bt_setup_machine_iterator_next(iter_ptr)==NULL, NULL);
}
END_TEST

TCase *bt_setup_example_tcase(void) {
  TCase *tc = tcase_create("bt_setup example case");

  tcase_add_test(tc,test_btsetup_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_example_suite(void) { 
  Suite *s=suite_create("BtSetupExample"); 

  suite_add_tcase(s,bt_setup_example_tcase());
  return(s);
}
