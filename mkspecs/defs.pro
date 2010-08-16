QMAKE_CXXFLAGS_DEBUG += 
QMAKE_CXXFLAGS += 

# detect cam_rvt support
rvt_cam {
  exists(../qcadrvtcam) {
    DEFINES += RVT_CAM
    INCLUDEPATH += ../../qcadrvtcam/include
    LIBS += -lpython 
  }
}

