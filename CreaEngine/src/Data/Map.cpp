#include "stdafx.h"

#include "Data\Map.h"
#include "Data\Node.h"
#include "json\json.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "Graphics\Sprite.h"

namespace crea
{
	Map::Map()
	{
		m_nWidth = 0;
		m_nHeight = 0;
		m_nTileWidth = 0;
		m_nTileHeight = 0;
		m_bIsGrid8 = false;
		m_pTerrainTileSet = nullptr;
		m_pGM = crea::GameManager::getSingleton();
	}

	Map::~Map()
	{
		clear();
		m_nWidth = 0;
		m_nHeight = 0;
	}

	bool Map::loadFromFileJSON(string& _filename)
	{
		Json::Value root;
		std::ifstream mapStream(_filename, std::ifstream::binary);
		if (mapStream.fail())
		{
			cerr << "Can't open map file: " << _filename << endl;
			return false;
		}

		setName(_filename);

		// Parse file
		mapStream >> root;

		int version = root.get("version", 0).asInt();
		if (version != 1)
		{
			cerr << "Can't parse map if version != 1" << endl;
			return false;
		}

		int iWidth = root.get("width", 10).asInt();
		int iHeight = root.get("height", 10).asInt();
		// Create all nodes
		setSize(iWidth, iHeight);

		m_nTileWidth = root.get("tilewidth", 10).asInt();
		m_nTileHeight = root.get("tileheight", 10).asInt();
		
		//Tilesets (load tilesets first as layers will refer to them...)
		Json::Value tilesets = root["tilesets"];
		for (unsigned int iTileset = 0; iTileset < tilesets.size(); ++iTileset)
		{
			Json::Value tileset = tilesets[iTileset];

			// Create a tileset
			TileSet* pTileSet = new TileSet();

			pTileSet->m_nColumns = tileset["columns"].asInt();
			pTileSet->m_nFirstgid = tileset["firstgid"].asInt();
			string image = tileset["image"].asString();
			pTileSet->m_nImageheight = tileset["imageheight"].asInt();
			pTileSet->m_nImagewidth = tileset["imagewidth"].asInt();
			pTileSet->m_nMargin = tileset["margin"].asInt();
			pTileSet->m_szName = tileset["name"].asString();
			pTileSet->m_nSpacing = tileset["spacing"].asInt();
			pTileSet->m_nTilecount = tileset["tilecount"].asInt();
			pTileSet->m_nTileheight = tileset["tileheight"].asInt();
			pTileSet->m_nTilewidth = tileset["tilewidth"].asInt();

			// terrain 
			Json::Value terrains = tileset["terrains"];
			if (terrains.size() != 0)
			{
				m_pTerrainTileSet = pTileSet;
			}
			for (unsigned int iTerrain = 0; iTerrain < terrains.size(); ++iTerrain)
			{
				Json::Value terrain = terrains[iTerrain];
				Terrain* pTerrain = new Terrain();
				pTerrain->m_szName = terrain["name"].asString();
				pTerrain->m_nTile = terrain["tile"].asInt();
				pTileSet->m_Terrains.push_back(pTerrain);
			}

			// Load Image and create sprite
			pTileSet->m_pSprite = m_pGM->getSprite(pTileSet->m_szName);
			pTileSet->m_pSprite->setTexture(m_pGM->getTexture(image));

			//  Tiles
			if (pTileSet->m_nTilecount > 1)
			{
				Json::Value tiles = tileset["tiles"];
				// CB: nothing done with tile information for now...
			}

			m_TileSet.push_back(pTileSet);
		}

		// Layers
		Json::Value layers = root["layers"];
		for (unsigned int iLayer = 0; iLayer < layers.size(); ++iLayer)
		{
			Json::Value layer = layers[iLayer];
			int iHeight = layer["height"].asInt();
			int iWidth = layer["width"].asInt();
			int iX = layer["x"].asInt();
			int iY = layer["y"].asInt();
			string type = layer["type"].asString();
			if (type == "tilelayer")
			{
				string szLayerName = layer["name"].asString();
				Json::Value data = layer["data"];
				for (short i = 0; i < iWidth; i++)
				{
					for (short j = 0; j < iHeight; j++)
					{
						short nIndex = (short)data[j*iWidth + i].asInt();
						m_Grid[i][j]->setTileTerrainId(nIndex);
					}
				}
			}
			if (type == "objectgroup")
			{
				// TD Agents
				// A compléter
				for (int i = 1; i < root["tilesets"].size(); i++)
				{
					// look in tilesets between 2 3 4
					// get name and sprite + gid
					// retrieve position in objects with gid

					int entityGid = root["tilesets"][i]["firstgid"].asInt();
					Entity* pEntity = m_pGM->getEntity(root["tilesets"][i]["name"].asString());

					for (int j = 0; j < layer["objects"].size(); j++)
					{
						if (layer["objects"][j]["gid"].asInt() == entityGid)
						{
							crea::Vector2f entityPosition = crea::Vector2f(layer["objects"][j]["x"].asFloat(), layer["objects"][j]["y"].asFloat() - layer["objects"][j]["height"].asFloat());
							pEntity->setPosition(entityPosition);
							m_pGM->addEntity(pEntity);
							break;
						}
					}

					// Load entity from file
					if (!root["tilesets"][i]["tileproperties"].isNull())
					{
						Json::Value agentProperty = root["tilesets"][i]["tileproperties"]["0"];
						if (!agentProperty["Entity"].isNull())
						{
							pEntity->loadFromFileJSON(DATAAGENTPATH + agentProperty["Entity"].asString());
							cout << "Dexterity:" << pEntity->getComponent<Agent>()->getDexterity() << endl;
							cout << "Health:" << pEntity->getComponent<Agent>()->getHealth() << endl;
							cout << "Intelligence:" << pEntity->getComponent<Agent>()->getIntelligence() << endl;
							cout << "Strength:" << pEntity->getComponent<Agent>()->getStrength() << endl;
						}
					}
					// If no entity file found, sprite renderer is loaded directly
					else
					{
						SpriteRenderer* pSpriteRenderer = new crea::SpriteRenderer();
						pEntity->addComponent(pSpriteRenderer);
						string textureName = root["tilesets"][i]["image"].asString();
						size_t last = textureName.find_last_of('/');
						textureName = textureName.substr(last + 1);
						Texture* pTexture = m_pGM->getTexture(textureName);

						Sprite* pSprite = new Sprite();
						pSprite->setTexture(pTexture);
						pSpriteRenderer->setSprite(pSprite);
					}

				}
			}

			// Properties
			//pEntity->loadFromFileJSON("name.ent");

		}

		return true;
	}

	void Map::setSize(short _nWidth, short _nHeight)
	{
		clear();
		m_nWidth = _nWidth;
		m_nHeight = _nHeight;
		m_Grid = new Node**[m_nWidth];
		for (short i = 0; i < m_nWidth; i++)
		{
			m_Grid[i] = new Node*[m_nHeight];
			for (short j = 0; j < m_nHeight; j++)
			{
				m_Grid[i][j] = new Node(i, j);
			}
		}

		// Set Neighbors
		for (short i = 0; i < m_nWidth; i++)
		{
			for (short j = 0; j < m_nHeight; j++)
			{
				if (j != 0)
				{
					m_Grid[i][j]->addChild(m_Grid[i][j - 1]); // top node
				}
				if (m_bIsGrid8 && j != 0 && i != m_nWidth - 1)
				{
					m_Grid[i][j]->addChild(m_Grid[i + 1][j - 1]); // top-right node
				}
				if (i != m_nWidth - 1)
				{
					m_Grid[i][j]->addChild(m_Grid[i + 1][j]); // right node
				}
				if (m_bIsGrid8 && i != m_nWidth - 1 && j != m_nHeight - 1)
				{
					m_Grid[i][j]->addChild(m_Grid[i + 1][j + 1]); // bottom-right node
				}
				if (j != m_nHeight - 1)
				{
					m_Grid[i][j]->addChild(m_Grid[i][j + 1]); // bottom node
				}
				if (m_bIsGrid8 && i != 0 && j != m_nHeight - 1)
				{
					m_Grid[i][j]->addChild(m_Grid[i - 1][j + 1]); // bottom-left node
				}
				if (i != 0)
				{
					m_Grid[i][j]->addChild(m_Grid[i - 1][j]); // left node
				}
				if (m_bIsGrid8 && i != 0 && j != 0)
				{
					m_Grid[i][j]->addChild(m_Grid[i - 1][j - 1]); // top-left node
				}
			}
		}

	}

	TileSet* Map::getTileSet(short _gid)
	{
		TileSet* pTileSet = nullptr;
		for (short i = 0; i < (short) m_TileSet.size(); i++)
		{
			pTileSet = m_TileSet[i];
			if (_gid >= pTileSet->m_nFirstgid && _gid < pTileSet->m_nFirstgid + pTileSet->m_nTilecount)
			{
				return pTileSet;
			}
		}
		return pTileSet;
	}

	Node* Map::getNodeAtPosition(Vector2f _v) 
	{ 
		int i = (int)_v.getX() / m_nTileWidth;
		int j = (int)_v.getY() / m_nTileHeight;
		if (i >= 0 && i < m_nWidth && j >= 0 && j < m_nHeight)
			return m_Grid[i][j];
		else
			return nullptr;
	}

	Vector2f Map::getNodePositionFromPixels(Vector2f _v)
	{
		int i = (int)_v.getX() / m_nTileWidth;
		int j = (int)_v.getY() / m_nTileHeight;
		if (i >= 0 && i < m_nWidth && j >= 0 && j < m_nHeight)
			return Vector2f((float)i, (float)j);
		else
			return Vector2f(0.f, 0.f);
	}

	Vector2f Map::getPixelsFromNodePosition(Vector2f _v)
	{
		int i = (int)_v.getX() * m_nTileWidth;
		int j = (int)_v.getY() * m_nTileHeight;
		return Vector2f((float)i, (float)j);
	}

	bool Map::update()
	{
		for (short i = 0; i < m_nWidth; i++)
		{
			for (short j = 0; j < m_nHeight; j++)
			{
				m_Grid[i][j]->update();
			}
		}
		return true;
	}

	bool Map::draw()
	{
		int tileid = 0, w = 0, h = 0, x = 0, y = 0;

		// Camera/Window restriction
		IntRect r = m_pGM->getWindowRect();
		int iMin = (int)r.getLeft() / m_nTileWidth;
		int iMax = (int)(r.getLeft() + r.getWidth()) / m_nTileWidth;
		int jMin = (int)r.getTop() / m_nTileHeight;
		int jMax = (int)(r.getTop()+r.getHeight()) / m_nTileHeight;

		// Protection if map smaller than window
		iMax = MIN(iMax, m_nWidth-1);
		jMax = MIN(jMax, m_nHeight-1);

		TileSet* pTileSet = m_pTerrainTileSet;
		for (short i = iMin; i <= iMax; i++)
		{
			Node** line = m_Grid[i];
			for (short j = jMin; j <= jMax; j++)
			{
				Node* pNode = line[j];
				tileid = pNode->getTileTerrainId(); // -1; // 30 -> 29
				
				IntRect iRect = pTileSet->getTextureRect(tileid);
				pTileSet->m_pSprite->setTextureRect(iRect.getLeft(), iRect.getTop(), iRect.getWidth(), iRect.getHeight());
				pTileSet->m_pSprite->setPosition((float)i*pTileSet->m_nTilewidth, (float)j*pTileSet->m_nTileheight);
				pTileSet->m_pSprite->draw();
			}
		}

		return true;
	}

	void Map::clear()
	{
		// Grid
		for (short i = 0; i < m_nWidth; i++)
		{
			for (short j = 0; j < m_nHeight; j++)
			{
				delete m_Grid[i][j];
			}
			delete[] m_Grid[i];
		}
		delete[] m_Grid;

		// Tilesets
		TileSet* pTileSet = nullptr;
		for (short i = 0; i < (short)m_TileSet.size(); i++)
		{
			pTileSet = m_TileSet[i];
			delete pTileSet;
		}
		m_TileSet.clear();
		m_pTerrainTileSet = nullptr;
	}
} // namespace crea
