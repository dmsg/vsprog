/***************************************************************************
 *   Copyright (C) 2009 - 2010 by Simon Qian <SimonQian@SimonQian.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "app_cfg.h"
#include "app_type.h"
#include "app_io.h"
#include "app_log.h"
#include "app_err.h"

#include "strparser.h"

#include "programmer.h"
#include "scripts.h"
#include "app_scripts.h"

#include "dal/mal/mal.h"

#if DAL_MIC2826_EN
#include "dal/mic2826/mic2826_drv.h"
#include "mic2826_script/mic2826_script.h"
#endif

#if DAL_NRF24L01_EN
#include "dal/nrf24l01/nrf24l01_drv.h"
#endif

VSS_HANDLER(dal_vss_init);
VSS_HANDLER(dal_vss_fini);
struct vss_cmd_t app_cmd[] = 
{
	VSS_CMD(	"dal.init",
				"initialize driver abstraction layer, format: dal.init KHZ",
				dal_vss_init),
	VSS_CMD(	"dal.fini",
				"finialize driver abstraction layer, format: dal.fini",
				dal_vss_fini),
#if DAL_MIC2826_EN
	VSS_CMD(	"mic2826.init",
				"initialize mic2826 through IIC, format: mic2826.init",
				mic2826_vss_init),
	VSS_CMD(	"mic2826.fini",
				"finalize mic2826 through IIC, format: mic2826.fini",
				mic2826_vss_fini),
	VSS_CMD(	"mic2826.config",
				"config mic2826 through IIC, format: "
				"mic2826.config DCDC LDO1 LOD2 LDO3",
				mic2826_vss_config),
#endif
	VSS_CMD_END
};

struct interfaces_info_t *interfaces = NULL;
RESULT dal_init(struct interfaces_info_t *ifs)
{
	interfaces = ifs;
	return ERROR_OK;
}

RESULT dal_commit(void)
{
	return interfaces->peripheral_commit();
}

RESULT dal_config_interface(char *dal_name, char *ifs, struct dal_info_t *info)
{
	uint32_t i;
	uint32_t size;
	struct dal_driver_t *d = NULL;
	uint8_t *buff = NULL;
	RESULT ret = ERROR_OK;
	
	i = 0;
	while (dal_drivers[i] != NULL)
	{
		if (!strcmp(dal_drivers[i]->name, dal_name))
		{
			d = dal_drivers[i];
			break;
		}
		i++;
	}
	if (NULL == d)
	{
		return ERROR_FAIL;
	}
	
	size = strparser_getsize(d->ifs_format);
	if (size > 1024)
	{
		LOG_WARNING("ifs_format too large: %d bytes.", size);
	}
	
	buff = (uint8_t *)malloc(size);
	if (NULL == buff)
	{
		return ERROR_FAIL;
	}
	
	if ((ERROR_OK != strparser_parse(ifs, d->ifs_format, buff, size)) || 
		(ERROR_OK != d->parse_interface(info, buff)))
	{
		ret = ERROR_FAIL;
	}
	free(buff);
	buff = NULL;
	return ret;
}

VSS_HANDLER(dal_vss_init)
{
	struct interfaces_info_t *ifs = NULL;
	
	VSS_CHECK_ARGC(1);
	if ((ERROR_OK != interface_assert(&ifs)) || (NULL == ifs))
	{
		LOG_ERROR(ERRMSG_FAILURE_HANDLE_DEVICE, "assert", "interface module");
		return ERROR_FAIL;
	}
	
	return dal_init(ifs);
}

VSS_HANDLER(dal_vss_fini)
{
	VSS_CHECK_ARGC(1);
	return ERROR_OK;
}

