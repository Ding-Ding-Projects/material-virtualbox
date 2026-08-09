/* $Id$ */
/** @file
 * VBox Qt GUI - anchored Material 3 element appearance editor.
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3AppearanceEditor_h
#define FEQT_INCLUDED_SRC_md3_UIMd3AppearanceEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QString>

#include "UILibraryDefs.h"

class QWidget;

/** Small non-modal editor for one persisted Material 3 element override. */
class SHARED_LIBRARY_STUFF UIMd3AppearanceEditor
{
public:

    /** Opens the editor beside @a pAnchor and returns focus to it on close. */
    static void open(QWidget *pAnchor, const QString &strAppearanceKey);
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3AppearanceEditor_h */
