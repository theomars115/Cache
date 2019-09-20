/*
Name: Dakoda Patterson
Class: CS 3421 
Assignment: Assignment 7
Compile: " g++ cache.cpp -o cache" 
Run: "./cache < trace.dat > trace.stats "
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

size_t numSets; 
size_t assocLevel;
size_t lineSize;
size_t numberOfDataLines;

size_t numEntries;

const size_t MAX_SETS = 8192;
const size_t MAX_ASSOCIATIVITY = 8;

class CacheBlock
{
public:
    CacheBlock()
    {
        blockData.LRU = 0;
        blockData.Data = 0;
        blockData.Tag = 0;
        blockData.dirtyBit = 0;
        blockData.validBit = 0;
    };
    
    CacheBlock(size_t init)
    {
        blockData.LRU = init;
        blockData.Data = init;
        blockData.Tag = init;
        blockData.dirtyBit = init;
        blockData.validBit = init;
    }
    
    ~CacheBlock()
    {};
    
    CacheBlock(const CacheBlock &c2)
    {
        blockData.LRU = c2.blockData.LRU;
        blockData.Data = c2.blockData.Data;
        blockData.Tag = c2.blockData.Tag;
        blockData.dirtyBit = c2.blockData.dirtyBit;
        blockData.validBit = c2.blockData.validBit;
    }
    
    void EraseMembers()
    {
        blockData.LRU = 0;
        blockData.Data = 0;
        blockData.Tag = 0;
        blockData.dirtyBit = 0;
        blockData.validBit = 0;
    }
    
    struct {
        unsigned short LRU;
        unsigned short Data;
        unsigned short Tag;
        unsigned short dirtyBit;
        unsigned short validBit;
    } blockData;
    
};

typedef std::vector<CacheBlock> cacheSet;
typedef std::vector<cacheSet> setAssociation;

void ReadConfig();
void PrintConfig();
size_t ReadDataTrace(std::vector<std::string> &);
void ParseDataTrace(size_t);
void ModeValidityCheck(const char);
bool SizeCheck(const unsigned int);
bool AlignmentCheck(const unsigned int,const unsigned int);
void PrintSummary(size_t,size_t,size_t);


int main()
{
    ReadConfig();
    PrintConfig();
    
    std::vector<std::string> traceDat;
    numberOfDataLines = ReadDataTrace(traceDat);
    
    CacheBlock defaultBlock(0); 
    setAssociation cacheAssociation(MAX_ASSOCIATIVITY, cacheSet(MAX_SETS,defaultBlock));
    
    char            mode;
    unsigned int    dataSize;
    unsigned int    address;
    bool            sizeError;
    bool            alignmentError;
    unsigned int    index;
    unsigned int    offset;
    unsigned int    tag;
    size_t          hitCounter = 0;
    size_t          missCounter = 0;
    size_t          refCounter = 0; 
    
    std::cout << "Results for Each Reference\n\n";
    std::cout << "Ref  Access Address    Tag   Index Offset Result Memrefs\n";
    std::cout << "---- ------ -------- ------- ----- ------ ------ -------\n";

    for (size_t programLine = 0; programLine < numberOfDataLines; ++programLine)
    {
        sscanf(traceDat[programLine].c_str(),"%c:%d:%x", &mode, &dataSize, &address);
        
        ModeValidityCheck(mode);
        
        sizeError = SizeCheck(dataSize);
        if(sizeError)
        {
            std::cerr << "line " << (programLine + 1) << " has illegal size " << dataSize << "\n";
            continue; 
        }
        
        alignmentError = AlignmentCheck(dataSize, address);
        if (alignmentError)
        {
            std::cerr << "line " << (programLine + 1) << " has misaligned reference at address " << std::hex << address << " for size " <<
                std::dec << dataSize << "\n";
            continue;
        }
        
        ++refCounter;

        unsigned int tempAddress = address;
        
        unsigned int offsetBitMask = static_cast<unsigned int>(lineSize - 1);
        unsigned int offsetShamt = static_cast<unsigned int>(log2(lineSize));
        offset = tempAddress & offsetBitMask;   
        tempAddress = tempAddress >> offsetShamt;
        
        unsigned int indexBitMask = static_cast<unsigned int>(numSets - 1);
        unsigned int indexShamt = static_cast<unsigned int>(log2(numSets));
        index = tempAddress & indexBitMask;      
        tempAddress = tempAddress >> indexShamt;
        
        tag = tempAddress;
       
        bool isThere = 0;
        unsigned int memrefs = 0;
        unsigned int tempTag = 0;
        bool isValid = 0;
        size_t hitSet = 0;
        
        memrefs = 0;
        
        for (size_t i = 0; i < assocLevel; ++i)
        {
            isValid = (cacheAssociation[i])[index].blockData.validBit;
            tempTag = (cacheAssociation[i])[index].blockData.Tag;
            if (isValid && (tempTag == tag))
            {
                isThere = 1;
                hitSet = i;
            }
        }
 
        if (isThere)
        {
            if (mode == 'R' || mode == 'r')
            {
                if (cacheAssociation[hitSet][index].blockData.LRU != (assocLevel - 1))
                {
                    unsigned int LRU_Test = cacheAssociation[hitSet][index].blockData.LRU;
                    for (size_t i = 0; i < assocLevel; ++i) 
                    {
                        if (cacheAssociation[i][index].blockData.LRU > LRU_Test)
                            --(cacheAssociation[i][index].blockData.LRU);
                    }
                    cacheAssociation[hitSet][index].blockData.LRU = (assocLevel - 1); 
                }
            }
            
            else if (mode == 'W' || mode == 'w')
            {
                cacheAssociation[hitSet][index].blockData.dirtyBit = 1;
                cacheAssociation[hitSet][index].blockData.validBit = 1;
                cacheAssociation[hitSet][index].blockData.Tag      = tag;
                cacheAssociation[hitSet][index].blockData.Data     = 0;
                
                if (cacheAssociation[hitSet][index].blockData.LRU != (assocLevel - 1))
                {
                    unsigned int LRU_Test = cacheAssociation[hitSet][index].blockData.LRU;
                    for (size_t i = 0; i < assocLevel; ++i)
                    {
                        if (cacheAssociation[i][index].blockData.LRU > LRU_Test)
                            --(cacheAssociation[i][index].blockData.LRU);
                    }
                    cacheAssociation[hitSet][index].blockData.LRU = (assocLevel - 1); 
                }
            } 
            
            else
            {
                std::cerr << "A fatal error occurred in HIT where the mode was neither Read nor Write.\n";
                exit(EXIT_FAILURE);
            }
        }
        
        else 
        {
            memrefs = 1; 
            
            if (mode == 'R' || mode == 'r') 
            {
                size_t indexToUse = 0;
                for (size_t i = 0; i < assocLevel; ++i)
                {
                    if (cacheAssociation[i][index].blockData.LRU == 0)
                    {
                        indexToUse = i;
                        break;
                    }
                }
                
                if (cacheAssociation[indexToUse][index].blockData.dirtyBit == 1)
                {
                    memrefs = 2;
                    cacheAssociation[indexToUse][index].blockData.dirtyBit = 0;
                }
               
                cacheAssociation[indexToUse][index].blockData.validBit = 1; 
                cacheAssociation[indexToUse][index].blockData.Tag = tag;
                cacheAssociation[indexToUse][index].blockData.Data = 0;
                
                for (unsigned int i = 0; i < assocLevel; ++i)
                {
                    if (cacheAssociation[i][index].blockData.LRU > 0)
                        --(cacheAssociation[i][index].blockData.LRU);
                }
                
                cacheAssociation[indexToUse][index].blockData.LRU = (assocLevel - 1);
            } 
            
            else if (mode == 'W' || mode == 'w')
            {
                size_t indexToUse = 0; 
                for (size_t i = 0; i < assocLevel; ++i)
                {
                    if (cacheAssociation[i][index].blockData.LRU == 0)
                    {
                        indexToUse = i;
                        break;  
                    }
                }
                
                if (cacheAssociation[indexToUse][index].blockData.dirtyBit == 1)
                {
                    memrefs = 2;
                }
                
                cacheAssociation[indexToUse][index].blockData.dirtyBit = 1; 
                
                cacheAssociation[indexToUse][index].blockData.validBit = 1;
                cacheAssociation[indexToUse][index].blockData.Tag = tag;
                cacheAssociation[indexToUse][index].blockData.Data = 0;
                
                for (unsigned int i = 0; i < assocLevel; ++i)
                {
                    if (cacheAssociation[i][index].blockData.LRU > 0)
                        --(cacheAssociation[i][index].blockData.LRU);
                }
                
                cacheAssociation[indexToUse][index].blockData.LRU = (assocLevel - 1);
            } 
            
            else
            {
                std::cerr << "A fatal error occurred in MISS where the mode was neither R nor W.\n";
                exit(EXIT_FAILURE);
            }
        }
        
        std::cout << std::right; 
        std::cout << std::setw(4) << refCounter;
        std::cout << std::setw(7);
        if (mode == 'R' || mode == 'r')
            std::cout << "read";
        if (mode == 'W' || mode == 'w')
            std::cout << "write";
        std::cout << std::setw(9) << std::hex << address;
        std::cout << std::setw(8) << std::hex << tag;
        std::cout << std::setw(6) << std::dec << index;
        std::cout << std::setw(7) << offset;
        std::cout << std::setw(7);
        if (isThere) 
        {
            std::cout << "hit";
            ++hitCounter;
        }
        else
        {
            std::cout << "miss";
            ++missCounter;
        }
        std::cout << std::setw(8) << memrefs;
        std::cout << "\n";

    } 
    
    PrintSummary(hitCounter,missCounter,refCounter);
}

void ReadConfig()
{
    std::ifstream inConfigFile("trace.config",std::ios::in);
    if (!inConfigFile)
    {
        std::cerr << "Configuration file could not be opened\n";
        exit(EXIT_FAILURE);
    }
    inConfigFile.seekg(std::ios::beg);
    
    char searchChar = '\0';
    
    while (searchChar != ':')
    {
        searchChar = inConfigFile.get();
    }
    inConfigFile >> numSets;
    searchChar = '\0';
    
    while (searchChar != ':')
    {
        searchChar = inConfigFile.get();
    }
    inConfigFile >> assocLevel; 
    searchChar = '\0';
    
    while (searchChar != ':')
    {
        searchChar = inConfigFile.get();
    }
    inConfigFile >> lineSize;
    searchChar = '\0';
    
    inConfigFile.close();
}



void PrintConfig()
{
    std::cout << "Cache Configuration\n\n";
    std::cout << "   " << numSets << " " << assocLevel << "-way set associative entries\n";
    std::cout << "   of line size " << lineSize << " bytes\n\n\n";
}



void PrintSummary(size_t hitCounter, size_t missCounter, size_t refCounter)
{
    std::cout << "\n\nSimulation Summary Statistics\n";
    std::cout << "-----------------------------\n";
    std::cout << "Total hits       : " << hitCounter << "\n";
    std::cout << "Total misses     : " << missCounter << "\n";
    std::cout << "Total accesses   : " << refCounter << "\n";
    float hitRatio = static_cast<float>(hitCounter) / refCounter;
    float missRatio = static_cast<float>(missCounter) / refCounter;
    std::cout << "Hit ratio        : " << std::fixed << std::setprecision(6) << hitRatio << "\n";
    std::cout << "Miss ratio       : " << std::fixed << std::setprecision(6) << missRatio << "\n\n";
}



size_t ReadDataTrace(std::vector<std::string> & traceDat)
{
  
    std::streambuf *cinBuf, *datBuf;
    cinBuf = std::cin.rdbuf(); 
    
    std::string inputLine;
    numberOfDataLines = 0;
    
    while (std::getline(std::cin, inputLine))
    {
        traceDat.push_back(inputLine);
        ++numberOfDataLines;
        
    }
    
    std::cin.rdbuf(cinBuf);
    return numberOfDataLines;
}

void ModeValidityCheck(const char mode)
{
    if (mode != 'R' && mode != 'W' && mode != 'r' && mode != 'w')
    {
        std::cerr << "Mode is invalid.  Exiting.\n";
        exit(EXIT_FAILURE);
    }
}

bool SizeCheck(const unsigned int size)
{
    bool sizeError = 0; 
    if (size != 1 && size != 2 && size != 4 && size != 8)
    {
        sizeError = 1;
    }
    return sizeError;
}

bool AlignmentCheck(const unsigned int size, const unsigned int address)
{
   
    if ((address % size) != 0)
        return 1;
    else
        return 0;
}