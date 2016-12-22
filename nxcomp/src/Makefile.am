NULL =

lib_LTLIBRARIES = libXcomp.la

libXcomp_la_SOURCES =					\
    ActionCache.cpp					\
    Agent.cpp						\
    Alpha.cpp						\
    Auth.cpp						\
    Bitmap.cpp						\
    BlockCache.cpp					\
    BlockCacheSet.cpp					\
    ChangeGC.cpp					\
    ChangeProperty.cpp					\
    ChannelCache.cpp					\
    Channel.cpp						\
    ChannelEndPoint.cpp					\
    CharCache.cpp					\
    Children.cpp					\
    ClearArea.cpp					\
    ClientCache.cpp					\
    ClientChannel.cpp					\
    ClientProxy.cpp					\
    ClientReadBuffer.cpp				\
    ClientStore.cpp					\
    Colormap.cpp					\
    ConfigureWindow.cpp					\
    Control.cpp						\
    CopyArea.cpp					\
    CreateGC.cpp					\
    CreatePixmap.cpp					\
    DecodeBuffer.cpp					\
    EncodeBuffer.cpp					\
    FillPoly.cpp					\
    Fork.cpp						\
    GenericChannel.cpp					\
    GenericReadBuffer.cpp				\
    GenericReply.cpp					\
    GenericRequest.cpp					\
    GetImage.cpp					\
    GetImageReply.cpp					\
    GetProperty.cpp					\
    GetPropertyReply.cpp				\
    ImageText16.cpp					\
    ImageText8.cpp					\
    IntCache.cpp					\
    InternAtom.cpp					\
    Jpeg.cpp						\
    Keeper.cpp						\
    List.cpp						\
    ListFontsReply.cpp					\
    Loop.cpp						\
    Message.cpp						\
    MD5.c						\
    Misc.cpp						\
    OpcodeStore.cpp					\
    Pack.c						\
    Pgn.cpp						\
    Pipe.cpp						\
    PolyArc.cpp						\
    PolyFillArc.cpp					\
    PolyFillRectangle.cpp				\
    PolyLine.cpp					\
    PolyPoint.cpp					\
    PolySegment.cpp					\
    PolyText16.cpp					\
    PolyText8.cpp					\
    Proxy.cpp						\
    ProxyReadBuffer.cpp					\
    PutImage.cpp					\
    PutPackedImage.cpp					\
    QueryFontReply.cpp					\
    ReadBuffer.cpp					\
    RenderAddGlyphs.cpp					\
    RenderChangePicture.cpp				\
    RenderComposite.cpp					\
    RenderCompositeGlyphs.cpp				\
    RenderCreateGlyphSet.cpp				\
    RenderCreatePicture.cpp				\
    RenderExtension.cpp					\
    RenderFillRectangles.cpp				\
    RenderFreeGlyphSet.cpp				\
    RenderFreePicture.cpp				\
    RenderGenericRequest.cpp				\
    RenderPictureClip.cpp				\
    RenderPictureFilter.cpp				\
    RenderPictureTransform.cpp				\
    RenderTrapezoids.cpp				\
    RenderTriangles.cpp					\
    Rgb.cpp						\
    Rle.cpp						\
    SendEvent.cpp					\
    SequenceQueue.cpp					\
    ServerCache.cpp					\
    ServerChannel.cpp					\
    ServerProxy.cpp					\
    ServerReadBuffer.cpp				\
    ServerStore.cpp					\
    SetClipRectangles.cpp				\
    SetUnpackAlpha.cpp					\
    SetUnpackColormap.cpp				\
    SetUnpackGeometry.cpp				\
    ShapeExtension.cpp					\
    Socket.cpp						\
    Split.cpp						\
    StaticCompressor.cpp				\
    Statistics.cpp					\
    Timestamp.cpp					\
    TranslateCoords.cpp					\
    Transport.cpp					\
    Unpack.cpp						\
    Vars.c						\
    Version.c						\
    WriteBuffer.cpp					\
    XidCache.cpp					\
    Z.cpp						\
    Log.cpp						\
    $(NULL)

libXcomp_la_LIBADD =					\
    @JPEG_LIBS@						\
    @PNG_LIBS@						\
    @Z_LIBS@						\
    @PTHREAD_LIBS@					\
    $(NULL)

AM_CXXFLAGS = 						\
    $(BASE_CXXFLAGS)					\
    $(JPEG_CFLAGS)					\
    $(PNG_CFLAGS)					\
    $(Z_CFLAGS)						\
    $(PTHREAD_CFLAGS)					\
    $(NULL)

AM_CPPFLAGS =						\
    -I$(top_srcdir)/include				\
    $(NULL)

libXcomp_la_LDFLAGS =					\
    -version-number @LT_COMP_VERSION@			\
    -no-undefined					\
    $(PTHREAD_LDFLAGS)					\
    $(NULL)

libXcompincludedir = $(includedir)/nx
libXcompinclude_HEADERS =				\
    $(top_srcdir)/include/MD5.h				\
    $(top_srcdir)/include/NX.h				\
    $(top_srcdir)/include/NXalert.h			\
    $(top_srcdir)/include/NXpack.h			\
    $(top_srcdir)/include/NXproto.h			\
    $(top_srcdir)/include/NXvars.h			\
    $(NULL)
