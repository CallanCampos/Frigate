#ifndef POKEBW2_FIELD_MAILVIEW_H
#define POKEBW2_FIELD_MAILVIEW_H

#include "types.h"
#include "field/mailsys.h"

// The mail screen's graphics (overlay 215's mailview.c): loads them over several calls, and frees them

// Returns TRUE once everything is loaded
BOOL MailView_Load(MailSysWork *work);
BOOL MailView_Unload(MailSysWork *work);

#endif // POKEBW2_FIELD_MAILVIEW_H
