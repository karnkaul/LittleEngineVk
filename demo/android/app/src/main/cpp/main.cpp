//
// Created by karnage on 11/14/20.
//

#include <android_native_app_glue.h>
#include <demo.hpp>

void android_main(android_app* pApp) {
	using namespace le;
	io::AAssetReader reader(pApp);
	if (!demo::run(reader, pApp)) { return; }
}
