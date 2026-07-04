/*
 * @file     cmd_fn.c
 * @brief    collection of executables functions from defined known_commands[]
 *
 * @author   Decawave Software
 *
 * @attention Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#include "cmd_fn.h"
#include "Generic.h"
//-----------------------------------------------------------------------------
const char CMD_FN_RET_OK[] = "ok\r\n";
const char CMD_FN_RET_ERR[] = "error\r\n";


/*
 * 功能:获取版本
 *
 * */
REG_FN(f_getver)
{
}


/*
 * 功能:保存
 *
 * */
REG_FN(f_save)
{
}

/*
 * 功能:保存
 *
 * */
REG_FN(f_saveR)
{
}


/*
 * 功能:恢复出厂模式
 *
 * */
REG_FN(f_rtoken)
{
}


/*
 * 功能:复位模块
 *
 * */
REG_FN(f_reset)
{
}


//-----------------------------------------------------------------------------
/*
 * 命令集
 */
//-----------------------------------------------------------------------------
const command_t known_commands []= {
	/* CMDNAME   MODE   fn     */

	{"SAVE",    f_save},					//保存
	{"SAVER",   f_saveR},					//保存+复位
	{"RESET",   f_reset}, 					//复位
	{"RTOKEN",  f_rtoken}, 					//恢复出厂模式
	{"GETVER",  f_getver},					//获取版本号

	{NULL,       NULL}
};
