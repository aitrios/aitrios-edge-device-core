/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NOTIFICATIONS_H__
#define __NOTIFICATIONS_H__

struct evp_agent_context;

int evp_agent_notifications_register(struct evp_agent_context *ctxt);

/* Agent status handler for LED management */
int agent_status_handler(const void *args, void *user_data);

#endif /* __NOTIFICATIONS_H__ */
