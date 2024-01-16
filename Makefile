TARGET := demo
CFLAGS += -I$(PWD)/../Include/DhopAI
CFLAGS += -I$(PWD)/../Include/DhopSdk/Event
CFLAGS += -I App/Src/$(plat)
CFLAGS += -litop_ai

IMAGE_PATH := /home/lukai/selected_data/images/test
DEST_PATH := $(APPROOT)/model/test

all:clean
	$(CROSS)g++ $(wildcard App/Src/*.c) $(wildcard App/Src/$(plat)/*.c)  $(CFLAGS) -o $(TARGET)

	mkdir -p $(APPROOT)/model
	@cp ./App/model/pest_yolov5_3516cv500_nnie.nnx $(APPROOT)/model

	mkdir -p $(APPROOT)/model/test
	
	@for i in $$(seq 0 799); do \
		cp $(IMAGE_PATH)/test_$$i.jpg $(DEST_PATH); \
	done

	mkdir -p $(APPROOT)/lib
	@cp $(PWD)/../Libs/*.so $(APPROOT)/lib

	@cp $(TARGET) 	$(APPROOT)
	@cp ./App/*.lic 	$(APPROOT)

	mkdir -p $(APPROOT)/web
	@cp ./App/web/* $(APPROOT)/web -r

	@cp ./App/json/$(plat)/sample.json $(TOOLPATH)
	@cd $(TOOLPATH)/; ./ezyBuilder build sample.json
clean:
	@rm -f ./$(TARGET)
	@rm -rf $(APPROOT)/*
	@rm -rf $(TOOLPATH)/*.json	
.PHONY:
	clean
