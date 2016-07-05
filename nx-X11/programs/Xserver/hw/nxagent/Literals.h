/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

/*
 * Simple table used to translate a request
 * opcode to the name of the X request.
 */

static char *nxagentRequestLiteral[] = 
{
  "None",
  "CreateWindow",
  "ChangeWindowAttributes",
  "GetWindowAttributes",
  "DestroyWindow",
  "DestroySubwindows",
  "ChangeSaveSet",
  "ReparentWindow",
  "MapWindow",
  "MapSubwindows",
  "UnmapWindow",
  "UnmapSubwindows",
  "ConfigureWindow",
  "CirculateWindow",
  "GetGeometry",
  "QueryTree",
  "InternAtom",
  "GetAtomName",
  "ChangeProperty",
  "DeleteProperty",
  "GetProperty",
  "ListProperties",
  "SetSelectionOwner",
  "GetSelectionOwner",
  "ConvertSelection",
  "SendEvent",
  "GrabPointer",
  "UngrabPointer",
  "GrabButton",
  "UngrabButton",
  "ChangeActivePointerGrab",
  "GrabKeyboard",
  "UngrabKeyboard",
  "GrabKey",
  "UngrabKey",
  "AllowEvents",
  "GrabServer",
  "UngrabServer",
  "QueryPointer",
  "GetMotionEvents",
  "TranslateCoords",
  "WarpPointer",
  "SetInputFocus",
  "GetInputFocus",
  "QueryKeymap",
  "OpenFont",
  "CloseFont",
  "QueryFont",
  "QueryTextExtents",
  "ListFonts",
  "ListFontsWithInfo",
  "SetFontPath",
  "GetFontPath",
  "CreatePixmap",
  "FreePixmap",
  "CreateGC",
  "ChangeGC",
  "CopyGC",
  "SetDashes",
  "SetClipRectangles",
  "FreeGC",
  "ClearArea",
  "CopyArea",
  "CopyPlane",
  "PolyPoint",
  "PolyLine",
  "PolySegment",
  "PolyRectangle",
  "PolyArc",
  "FillPoly",
  "PolyFillRectangle",
  "PolyFillArc",
  "PutImage",
  "GetImage",
  "PolyText8",
  "PolyText16",
  "ImageText8",
  "ImageText16",
  "CreateColormap",
  "FreeColormap",
  "CopyColormapAndFree",
  "InstallColormap",
  "UninstallColormap",
  "ListInstalledColormaps",
  "AllocColor",
  "AllocNamedColor",
  "AllocColorCells",
  "AllocColorPlanes",
  "FreeColors",
  "StoreColors",
  "StoreNamedColor",
  "QueryColors",
  "LookupColor",
  "CreateCursor",
  "CreateGlyphCursor",
  "FreeCursor",
  "RecolorCursor",
  "QueryBestSize",
  "QueryExtension",
  "ListExtensions",
  "ChangeKeyboardMapping",
  "GetKeyboardMapping",
  "ChangeKeyboardControl",
  "GetKeyboardControl",
  "Bell",
  "ChangePointerControl",
  "GetPointerControl",
  "SetScreenSaver",
  "GetScreenSaver",
  "ChangeHosts",
  "ListHosts",
  "SetAccessControl",
  "SetCloseDownMode",
  "KillClient",
  "RotateProperties",
  "ForceScreenSaver",
  "SetPointerMapping",
  "GetPointerMapping",
  "SetModifierMapping",
  "GetModifierMapping",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "NoOperation"
};

static char *nxagentRenderRequestLiteral[] =
{
  "RenderQueryVersion",
  "RenderQueryPictFormats",
  "RenderQueryPictIndexValues",
  "RenderQueryDithers",
  "RenderCreatePicture",
  "RenderChangePicture",
  "RenderSetPictureClipRectangles",
  "RenderFreePicture",
  "RenderComposite",
  "RenderScale",
  "RenderTrapezoids",
  "RenderTriangles",
  "RenderTriStrip",
  "RenderTriFan",
  "RenderColorTrapezoids",
  "RenderColorTriangles",
  "RenderTransform",
  "RenderCreateGlyphSet",
  "RenderReferenceGlyphSet",
  "RenderFreeGlyphSet",
  "RenderAddGlyphs",
  "RenderAddGlyphsFromPicture",
  "RenderFreeGlyphs",
  "RenderCompositeGlyphs",
  "RenderCompositeGlyphs",
  "RenderCompositeGlyphs",
  "RenderFillRectangles",
  "RenderCreateCursor",
  "RenderSetPictureTransform",
  "RenderQueryFilters",
  "RenderSetPictureFilter",
  "RenderCreateAnimCursor",
  "RenderAddTraps",
  "RenderCreateSolidFill",
  "RenderCreateLinearGradient",
  "RenderCreateRadialGradient",
  "RenderCreateConicalGradient"
};

static char *nxagentShmRequestLiteral[] =
{
  "ShmQueryVersion",
  "ShmAttach",
  "ShmDetach",
  "ShmPutImage",
  "ShmGetImage",
  "ShmCreatePixmap"
};

