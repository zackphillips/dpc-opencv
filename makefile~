CC=g++
IDIR=.
CFLAGS=-I$(IDIR) -ggdb

LIBS=-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_stitching

qDPC_desktop: qDPC.cpp
	$(CC) -o qDPC_desktop qDPC.cpp $(CFLAGS) $(LIBS)

clean:
	rm -rf qDPC_desktop
