
# This file is part of the Coriolis Software.
# Copyright (c) UPMC 2019-2019, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |          Alliance / Hurricane  Interface                        |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./etc/node600/phenitec/display.py"        |
# +-----------------------------------------------------------------+


import helpers.io
helpers.io.vprint( 2, '     - "%s".' % helpers.truncPath(__file__) )

import common.display


common.display.createStyles( scale=0.5 )