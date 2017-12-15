#!/bin/sh

# We want GNU indent, so first search for gindent to avoid /usr/bin/indent
# on the BSDs, which won't work for us
INDENT=`which gnuindent || which gindent || which indent`

if [ -z "${INDENT}" ] ; then
    echo "Could not find indent, sorry..." >&2
    exit 1
fi

$INDENT -linux -bad -bap -blf -bli0 -cbi0 -cdw -nce -cs -i4 -lc80 -psl -nbbo \
    -nbc -psl -nbfda -nut -nss -T pointer -T ScreenPtr -T ScrnInfoPtr -T pointer \
    -T DeviceIntPtr -T DevicePtr -T ClientPtr -T CallbackListPtr \
    -T CallbackProcPtr -T OsTimerPtr -T CARD32 -T CARD16 -T CARD8 \
    -T INT32 -T INT16 -T INT8 -T Atom -T Time -T WindowPtr -T DrawablePtr \
    -T PixmapPtr -T ColormapPtr -T CursorPtr -T Font -T XID -T Mask \
    -T BlockHandlerProcPtr -T WakeupHandlerProcPtr -T RegionPtr \
    -T InternalEvent -T GrabPtr -T Timestamp -T Bool -T TimeStamp \
    -T xEvent -T DeviceEvent -T RawDeviceEvent -T GrabMask -T Window \
    -T Drawable -T FontPtr -T CallbackPtr -T XIPropertyValuePtr \
    -T GrabParameters -T deviceKeyButtonPointer -T TouchOwnershipEvent \
    -T xGenericEvent -T DeviceChangedEvent -T GCPtr -T BITS32 \
    -T xRectangle -T BoxPtr -T RegionRec -T ValuatorMask -T KeyCode \
    -T KeySymsPtr -T XkbDescPtr -T InputOption -T XI2Mask -T DevUnion \
    -T DevPrivateKey -T DevScreenPrivateKey -T PropertyPtr -T RESTYPE \
    -T XkbAction -T XkbChangesPtr -T XkbControlsPtr -T PrivatePtr -T pmWait \
    -T _XFUNCPROTOBEGIN -T _XFUNCPROTOEND -T _X_EXPORT "$@"

