#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "ubasic/ubasic.h"

/* file name used for storing EEPROM data */
static const char *EEPROM_Filename = "EEPROM.bin";

/**
 * @brief Format the EEPROM
 */
static void eepromFormat(void)
{
    FILE *pFile = NULL;
    uint8_t buffer[256];
    unsigned i = 0;

    printf("EEPROM: creating file %s\n", EEPROM_Filename);
    pFile = fopen(EEPROM_Filename, "wb");
    if (!pFile)
    {
        perror("fopen error");
        return;
    }
    for (i = 0; i < 256; i++)
    {
        buffer[i] = 0xFF;
    }
    for (i = 0; i < 256; i++)
    {
        fwrite(buffer, 1, sizeof(buffer), pFile);
    }
    fclose(pFile);
}

/**
 * @brief Read some data from the EEPROM
 * @param start_address EEPROM starting memory address
 * @param buffer data to store
 * @param length number of bytes of data to read
 */
static size_t eepromRead(
    uint16_t start_address,
    uint8_t *buffer,
    uint16_t length)
{
    size_t bytes_read = 0, bytes_to_read = 0;
    FILE *pFile = NULL;
    long offset = 0;
    int seeking = 0;

    pFile = fopen(EEPROM_Filename, "rb");
    if (!pFile)
    {
        eepromFormat();
        pFile = fopen(EEPROM_Filename, "rb");
    }
    if (pFile)
    {
        offset = start_address;
        seeking = fseek(pFile, offset, SEEK_SET);
        if (seeking == 0)
        {
            bytes_to_read = length;
            bytes_read = fread(buffer, 1, bytes_to_read, pFile);
            if (bytes_read != bytes_to_read)
            {
                perror("fread error");
            }
            fclose(pFile);
        }
        else
        {
            perror("fseek error");
        }
    }

    return bytes_read;
}

/**
 * @brief Write some data to the EEPROM
 * @param start_address EEPROM starting memory address
 * @param buffer data to send
 * @param length number of bytes of data
 */
static size_t eepromWrite(
    uint16_t start_address,
    uint8_t *buffer,
    uint16_t length)
{
    size_t bytes_written = 0, bytes_to_write = 0;
    FILE *pFile = NULL;
    long offset = 0;
    int seeking = 0;

    pFile = fopen(EEPROM_Filename, "rb+");
    if (pFile)
    {
        offset = start_address;
        seeking = fseek(pFile, offset, SEEK_SET);
        if (seeking == 0)
        {
            bytes_to_write = length;
            bytes_written = fwrite(buffer, 1, bytes_to_write, pFile);
            if (bytes_written != bytes_to_write)
            {
                perror("fwrite error");
            }
            fclose(pFile);
        }
    }

    return bytes_written;
}

void EE_Init(void)
{
    /* nothing to do */
}

void EE_WriteVariable(uint8_t Name, uint8_t Vartype, uint8_t datalen_bytes, uint8_t *dataptr)
{
    uint16_t start_address = Name * (datalen_bytes + 2); // Calculate the starting address based on variable name
    uint8_t buffer[256];

    // Prepare the buffer with the variable type and data length
    buffer[0] = Vartype;       // First byte is the variable type
    buffer[1] = datalen_bytes; // Second byte is the data length
    for (uint8_t i = 0; i < datalen_bytes; i++)
    {
        buffer[i + 2] = dataptr[i]; // Copy the actual data into the buffer
    }

    // Write the buffer to EEPROM
    eepromWrite(start_address, buffer, datalen_bytes + 2);
}

void EE_ReadVariable(uint8_t Name, uint8_t Vartype, uint8_t *dataptr, uint8_t *datalen)
{
    uint16_t start_address = Name * (256); // Calculate the starting address based on variable name
    uint8_t buffer[256];

    // Read the data from EEPROM
    eepromRead(start_address, buffer, 256);

    // Check if the variable type matches
    if (buffer[0] == Vartype)
    {
        *datalen = buffer[1]; // Get the data length
        for (uint8_t i = 0; i < *datalen; i++)
        {
            dataptr[i] = buffer[i + 2]; // Copy the actual data into the provided pointer
        }
    }
    else
    {
        *datalen = 0; // If type does not match, set length to 0
    }
}

void EE_DumpFlash(void)
{
    uint8_t name = 0;
    uint8_t vartype = 0;
    uint8_t datalen = 0;
    uint8_t buffer[256];

    printf("EEPROM Dump:\n");
    for (name = 0; name < 255; name++)
    {
        EE_ReadVariable(name, vartype, buffer, &datalen);
        if (datalen > 0)
        {
            printf("Variable %c: Type=%d, Length=%d, Data=", name + 'a', vartype, datalen);
            for (uint8_t i = 0; i < datalen; i++)
            {
                printf("%02X ", buffer[i]);
            }
            printf("\n");
        }
    }
}
