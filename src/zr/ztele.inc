/*
 * ============================================================================
 *
 *   Zombie:Reloaded
 *
 *   File:          ztele.inc
 *   Type:          Module
 *   Description:   ZTele handle functions.
 *
 * ============================================================================
 */

/**
 * Array to store client's spawn location.
 */
new Float:g_vecZTeleSpawn[MAXPLAYERS + 1][3];

/**
 * Array to store client's current location.
 */
new Float:g_vecZTeleOrigin[MAXPLAYERS + 1][3];

/**
 * Array to store the tele count of each client.
 */
new g_iZTeleCount[MAXPLAYERS + 1];

/**
 * Array for storing ZTele timer handles per client.
 */
new Handle:tZTele[MAXPLAYERS + 1];

/**
 * Array to store time left before teleport.
 */
new g_iZTeleTimeLeft[MAXPLAYERS + 1];

/**
 * Client is joining the server.
 * 
 * @param client    The client index.
 */
ZTeleClientInit(client)
{
    // Reset timer handle.
    tZTele[client] = INVALID_HANDLE;
}

/**
 * Client is spawning into the game.
 * 
 * @param client    The client index.
 */
ZTeleOnClientSpawn(client)
{
    // Reset tele count.
    g_iZTeleCount[client] = 0;
    
    // Get spawn location.
    GetClientAbsOrigin(client, g_vecZTeleSpawn[client]);
    
    // If timer is running, kill it.
    if (tZTele[client] != INVALID_HANDLE)
    {
        KillTimer(tZTele[client]);
    }
    
    // Reset timer handle.
    tZTele[client] = INVALID_HANDLE;
}

/**
 * Client has been killed.
 * 
 * @param client    The client index.
 */
ZTeleOnClientDeath(client)
{
    // If timer is running, kill it.
    if (tZTele[client] != INVALID_HANDLE)
    {
        KillTimer(tZTele[client]);
    }
    
    // Reset timer handle.
    tZTele[client] = INVALID_HANDLE;
}

/**
 * Player has been infected.
 * 
 * @param client    The client index.
 */
ZTeleOnClientInfected(client)
{
    // If timer is running, kill it.
    if (tZTele[client] != INVALID_HANDLE)
    {
        KillTimer(tZTele[client]);
    }
    
    // Reset timer handle.
    tZTele[client] = INVALID_HANDLE;
}

/**
 * Teleports a client back to spawn if conditions are met.
 * 
 * @param client    The client index.
 * @return          True if teleport was successful, false otherwise. 
 */
bool:ZTeleClient(client)
{
    // If the client is dead, then stop.
    if (!IsPlayerAlive(client))
    {
        return false;
    }
    
    new bool:infected = InfectIsClientInfected(client);
    
    // If zombie cvar is disabled and the client is a zombie, then stop.
    new bool:ztelezombie = GetConVarBool(g_hCvarsList[CVAR_ZTELE_ZOMBIE]);
    if (infected && !ztelezombie)
    {
        // Tell client they must be human to use this feature.
        ZR_PrintToChat(client, "Must be human");
        return false;
    }
    
    // If zombie has spawned, get before value, get the after value otherwise.
    // If the cvar is disabled and the client is a human, then stop.
    new bool:ztelehuman = g_bZombieSpawned ? GetConVarBool(g_hCvarsList[CVAR_ZTELE_HUMAN_AFTER]) : GetConVarBool(g_hCvarsList[CVAR_ZTELE_HUMAN_BEFORE]);
    if (!infected && !ztelehuman)
    {
        // Tell client that feature is restricted at this time.
        ZR_PrintToChat(client, "ZTele restricted human");
        return false;
    }
    
    // If the tele limit has been reached, then stop.
    new ztelemax = infected ? GetConVarInt(g_hCvarsList[CVAR_ZTELE_MAX_ZOMBIE]) : GetConVarInt(g_hCvarsList[CVAR_ZTELE_MAX_HUMAN]);
    if (g_iZTeleCount[client] >= ztelemax)
    {
        // Tell client that they have already reached their limit.
        ZR_PrintToChat(client, "ZTele max", ztelemax);
        return false;
    }
    
    // If teleport is already in progress, then stop.
    if (tZTele[client] != INVALID_HANDLE)
    {
        ZR_PrintToChat(client, "ZTele in progress");
        return false;
    }
    
    // Get current location.
    GetClientAbsOrigin(client, g_vecZTeleOrigin[client]);
    
    // If timer is running, kill it.
    if (tZTele[client] != INVALID_HANDLE)
    {
        KillTimer(tZTele[client]);
    }
    
    // Set timeleft array to value of respective cvar.
    g_iZTeleTimeLeft[client] = infected ? GetConVarInt(g_hCvarsList[CVAR_ZTELE_DELAY_ZOMBIE]) : GetConVarInt(g_hCvarsList[CVAR_ZTELE_DELAY_HUMAN]);
    if (g_iZTeleTimeLeft[client] > 0)
    {
        // Tell client how much time is left until teleport.
        ZR_PrintCenterText(client, "ZTele countdown", g_iZTeleTimeLeft[client]);
        
        // Start repeating timer.
        tZTele[client] = CreateTimer(1.0, ZTeleTimer, client, TIMER_FLAG_NO_MAPCHANGE|TIMER_REPEAT);
    }
    else
    {
        // Reset timer handle.
        tZTele[client] = INVALID_HANDLE;
        
        // Teleport client.
        TeleportEntity(client, g_vecZTeleSpawn[client], NULL_VECTOR, NULL_VECTOR);
        
        // Tell client they've been teleported.
        ZR_PrintCenterText(client, "ZTele countdown end", g_iZTeleCount[client], ztelemax);
        
        // Increment teleport count.
        g_iZTeleCount[client]++;
    }
    
    return true;
}

/**
 * Timer callback, counts down teleport to the client.
 * 
 * @param timer     The timer handle.
 * @param client    The client index.
 */
public Action:ZTeleTimer(Handle:timer, any:client)
{
    // If client leaves, then stop timer.
    if (!IsClientInGame(client))
    {
        return Plugin_Stop;
    }
    
    new bool:zteleautocancel = GetConVarBool(g_hCvarsList[CVAR_ZTELE_AUTOCANCEL]);
    if (zteleautocancel)
    {
        // If client has been running around after using ZTele, then stop timer.
        new Float:vecClient[3];
        GetClientAbsOrigin(client, vecClient);
        
        new Float:distance = GetVectorDistance(vecClient, g_vecZTeleOrigin[client]);
        new Float:autocanceldistance = GetConVarFloat(g_hCvarsList[CVAR_ZTELE_AUTOCANCEL_DISTANCE]);
        
        // Convert cvar units
        new Float:unitdistance = ZRConvertUnitsFloat(autocanceldistance, CONVERSION_FEET_TO_UNITS);
        
        // Check if distance has been surpassed.
        if (distance > unitdistance)
        {
            // Reset timer handle.
            tZTele[client] = INVALID_HANDLE;
            
            // Tell client teleport has been cancelled.
            ZR_PrintCenterText(client, "ZTele autocancel centertext");
            ZR_PrintToChat(client, "ZTele autocancel text", RoundToNearest(autocanceldistance));
            
            // Stop timer.
            return Plugin_Stop;
        }
    }
    
    // Decrement time left.
    g_iZTeleTimeLeft[client]--;
    
    // Tell client how much time is left until teleport.
    ZR_PrintCenterText(client, "ZTele countdown", g_iZTeleTimeLeft[client]);
    
    // Time has expired.
    if (g_iZTeleTimeLeft[client] <= 0)
    {
        // Teleport client.
        TeleportEntity(client, g_vecZTeleSpawn[client], NULL_VECTOR, NULL_VECTOR);
        
        // Increment teleport count.
        g_iZTeleCount[client]++;
        
        // Get max teleports per round.
        new ztelemax = InfectIsClientInfected(client) ? GetConVarInt(g_hCvarsList[CVAR_ZTELE_MAX_ZOMBIE]) : GetConVarInt(g_hCvarsList[CVAR_ZTELE_MAX_HUMAN]);
        
        // Tell client spawn protection is over.
        ZR_PrintCenterText(client, "ZTele countdown end", g_iZTeleCount[client], ztelemax);
        
        // Clear timer handle.
        tZTele[client] = INVALID_HANDLE;
        
        // Stop timer.
        return Plugin_Stop;
    }
    
    // Allow timer to continue repeating.
    return Plugin_Continue;
}