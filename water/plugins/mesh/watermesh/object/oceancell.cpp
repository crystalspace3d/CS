/*
    Copyright (C) 2008 by Pavel Krajcevski

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "oceancell.h"
#include <csutil/sysfunc.h> // provides csPrintf();

using namespace CS::Plugins::WaterMesh;

csOceanCell::csOceanCell(float len, float wid, OceanLOD level)
{
  this->len = len;
  this->wid = wid;
  
  type = level;
  
  oHeight = 0;
  
  gc = csVector2(0, 0);
  
  isSetup = false;
  
  bufferHoldersNeedSetup = true;
  buffersNeedSetup = false;
}

csOceanCell::~csOceanCell()
{
    
}


void csOceanCell::SetupVertices()
{
	csDirtyAccessArray<csVector3> verts[16];
	csDirtyAccessArray<csVector3> norms[16];
	csDirtyAccessArray<csColor> cols[16];
	csDirtyAccessArray<csVector2> texs[16];
	csDirtyAccessArray<csTriangle> tris[16];

  if(!isSetup)
  {
    float gran;
    switch(type)
    {
      default:
      case LOD_LEVEL_1:
        gran = 0.1f;
        break;
      case LOD_LEVEL_2:
        gran = 0.2f;
        break;
      case LOD_LEVEL_3:
        gran = 0.4f;
        break;
      case LOD_LEVEL_4:
        gran = 0.8f;
        break;
      case LOD_LEVEL_5:
        gran = 1.6f;
        break;
    }

    uint maxjd = (uint) (len * gran);
    uint maxj = maxjd + 1;
    uint maxid = (uint) (wid * gran);
    uint maxi = maxid + 1;


	
	for (uint k = 0; k < 16 ; k++)
	{
	    for(uint j = 0; j < maxj ; ++j)
		{
		   for(uint i = 0; i < maxi  ; ++i)
		   {
		     verts[k].Push(csVector3 ((i * wid / maxid), oHeight, (j * len / maxjd)));
		     texs[k].Push(csVector2((i * wid / maxid) / 1.5, (j * len / maxjd) / 1.5));
			 cols[k].Push(csColor (0.17f,0.27f,0.26f));
			 norms[k].Push(csVector3 (0, 1, 0));
		   }
        }
		
		// The inner mesh
        for(uint j = 1; j <  maxjd-1  ; ++j)
        {
            for(uint i = 1; i < maxid-1 ; ++i)
            {
				 tris[k].Push(csTriangle ((int)(j * maxi + i), 
						 (int)((j + 1) * maxi + i), 
						 (int)(j * maxi + i + 1)));
				 tris[k].Push(csTriangle ((int)(j * maxi + i + 1),
						 (int)((j + 1) * maxi + i),
						 (int)((j + 1) * maxi + i + 1)));	
	        }
	    }
	}

	
	for (uint k = 0; k < 16 ; k++)
	{
		// The top mesh
		for ( uint j=maxid-1, i=1; i < maxid ; i++)
		{
			if(k/8)
			{
				// High res
				if(i%2)
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i),
						(int)((j + 1) * maxi + i - 1),
						(int)((j + 1) * maxi + i )));
					tris[k].Push(csTriangle ((int)(j * maxi + i),
						(int)((j + 1) * maxi + i),
						(int)((j + 1) * maxi + i + 1)));
					if (i!=maxid-1)
					{
						tris[k].Push(csTriangle ((int)(j * maxi + i ), 
							(int)((j + 1) * maxi + i + 1), 
							(int)(j * maxi + i + 1)));
					}
				}
				else 
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)((j + 1) * maxi + i), 
						(int)(j * maxi + i + 1)));
				}
			}
			else
			{
				// Low res
				if(i%2)
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i),
						(int)((j + 1) * maxi + i - 1),
						(int)((j + 1) * maxi + i + 1)));

					if (i!=maxid-1)
					{
						tris[k].Push(csTriangle ((int)(j * maxi + i ), 
							(int)((j + 1) * maxi + i + 1), 
							(int)(j * maxi + i + 1)));
					}

				}
				else
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)((j + 1) * maxi + i), 
						(int)(j * maxi + i + 1))); 
				}
			}
		}

		// The Right mesh
		for(uint j=1, i=maxid -1  ; j < maxjd ; j++ )
		{
			if((k/4)%2)
			{
				// High boundaries
				if (j%2)
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)(j * maxi + i + 1), 
						(int)((j - 1) * maxi + i + 1)));
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)((j + 1) * maxi + i + 1), 
						(int)(j * maxi + i + 1)));
					if (j!=maxjd-1)
					{
						tris[k].Push(csTriangle ((int)(j * maxi + i), 
							(int)((j + 1) * maxi + i), 
							(int)((j + 1) * maxi + i + 1)));
					}
				}
				else
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)((j + 1) * maxi + i), 
						(int)(j * maxi + i + 1)));
				}
			}
			else
			{

				// Low boundaries
				if (j%2)
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)((j + 1) * maxi + i + 1), 
						(int)((j - 1) * maxi + i + 1)));
					if (j!=maxjd-1)
					{
						tris[k].Push(csTriangle ((int)(j * maxi + i), 
							(int)((j + 1) * maxi + i), 
							(int)((j + 1) * maxi + i + 1)));
					}
				}
				else
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)((j + 1) * maxi + i), 
						(int)(j * maxi + i + 1)));
				}
			}
		}

		// Bottom mesh
		for(uint j=0, i=1 ; i < maxid ; i++ )
		{	
			if((k/2)%2)
			{
				// High res
				if(i%2)
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i - 1),
						(int)((j + 1) * maxi + i),
						(int)(j * maxi + i )));

					tris[k].Push(csTriangle ((int)(j * maxi + i ),
						(int)((j + 1) * maxi + i),
						(int)(j * maxi + i + 1)));
					if (i!=maxid-1)
					{
						tris[k].Push(csTriangle ((int)(j * maxi + i + 1), 
							(int)((j + 1) * maxi + i ), 
							(int)((j + 1) * maxi + i + 1)));
					}
				}
				else
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)((j + 1) * maxi + i), 
						(int)((j + 1) * maxi + i + 1)));  
				}
			}
			else
			{
				// Low res
				if(i%2)
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i - 1),
						(int)((j + 1) * maxi + i),
						(int)(j * maxi + i + 1)));
					if (i!=maxid-1)
					{
						tris[k].Push(csTriangle ((int)(j * maxi + i + 1), 
							(int)((j + 1) * maxi + i ), 
							(int)((j + 1) * maxi + i + 1)));
					}
				}
				else
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i), 
						(int)((j + 1) * maxi + i), 
						(int)((j + 1) * maxi + i + 1)));  
				}
			}
		}

		// Left mesh
		for(uint j=1, i=0 ; j < maxjd ; j++ )
		{
			if(k%2)
			{
				// High res
				if (j%2)
				{
					tris[k].Push(csTriangle ((int)( (j-1) * maxi + i ), 
						(int)(j * maxi + i), 
						(int)(j * maxi + i + 1)));	
					tris[k].Push(csTriangle ((int)(j * maxi + i ), 
						(int)((j+1) * maxi + i ), 
						(int)( j * maxi + i +1 )));	
					if (j!=maxjd-1)
					{
						tris[k].Push(csTriangle ((int)(j * maxi + i + 1),
							(int)((j + 1) * maxi + i),
							(int)((j + 1) * maxi + i + 1)));
					}
				}
				else
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i),
						(int)((j + 1) * maxi + i + 1),
						(int)(j * maxi + i + 1)));
				}
			}
			else
			{
				// Low res
				if (j%2)
				{
					tris[k].Push(csTriangle ((int)( (j-1) * maxi + i ), 
						(int)((j + 1) * maxi + i), 
						(int)(j * maxi + i + 1)));	
					if (j!=maxjd-1)
					{
						tris[k].Push(csTriangle ((int)(j * maxi + i + 1),
							(int)((j + 1) * maxi + i),
							(int)((j + 1) * maxi + i + 1)));
					}
				}
				else
				{
					tris[k].Push(csTriangle ((int)(j * maxi + i),
						(int)((j + 1) * maxi + i + 1),
						(int)(j * maxi + i + 1)));
				}
			}
		}
	}

	


	for (uint k = 0 ; k < 16 ; k++)
	{
		vertsARR.Push(verts[k]);
		colsARR.Push(cols[k]);
		texsARR.Push(texs[k]);
		normsARR.Push(norms[k]);
		trisARR.Push(tris[k]);
	}

    buffersNeedSetup = true;
    isSetup = true;
  }
}

void csOceanCell::SetupBuffers()
{
  if(!buffersNeedSetup)
    return;
  
  if (!vertex_bufferARR.GetSize())
  {
	  for(uint i = 0; i < 16; i++)
	  {
		 // Create a buffer that doesn't copy the data.
		 vertex_bufferARR.Push( csRenderBuffer::CreateRenderBuffer (
		 vertsARR[i].GetSize(), CS_BUF_STATIC, CS_BUFCOMP_FLOAT,
		 3));

		 vertex_bufferARR[i]->CopyInto (vertsARR[i].GetArray(), vertsARR[i].GetSize());
		}
  }


  if (!texel_bufferARR.GetSize())
  {
	  for(uint i = 0; i < 16; i++)
	  {
		// Create a buffer that doesn't copy the data.
		texel_bufferARR.Push( csRenderBuffer::CreateRenderBuffer (
		vertsARR[i].GetSize(), CS_BUF_STATIC, CS_BUFCOMP_FLOAT,
		2));

		texel_bufferARR[i]->CopyInto (texsARR[i].GetArray(), vertsARR[i].GetSize());
	  }
  }
  

  if (!index_bufferARR.GetSize())
  {
	 for(uint i = 0; i < 16; i++)
	 {
		  // Create a buffer that doesn't copy the data.
		  index_bufferARR.Push(  csRenderBuffer::CreateIndexRenderBuffer (
		  (trisARR[i].GetSize()*3),
		   CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_INT,
		   0, vertsARR[i].GetSize()-1) );

		  index_bufferARR[i]->CopyInto (trisARR[i].GetArray(), trisARR[i].GetSize()*3);
	 }
  }
  
  if (!normal_bufferARR.GetSize())
  {    
	  for(uint i = 0; i < 16; i++)
	  {
		// Create a buffer that doesn't copy the data.
		normal_bufferARR.Push( csRenderBuffer::CreateRenderBuffer (
        normsARR[i].GetSize(), CS_BUF_STATIC, CS_BUFCOMP_FLOAT,
        3));

		normal_bufferARR[i]->CopyInto (normsARR[i].GetArray(), normsARR[i].GetSize());
		}
		
  }

  if (!color_bufferARR.GetSize())
  {   
	  for(uint i = 0; i < 16; i++)
	  {
		// Create a buffer that doesn't copy the data.
		color_bufferARR.Push(  csRenderBuffer::CreateRenderBuffer (
        colsARR[i].GetSize(), CS_BUF_STATIC, CS_BUFCOMP_FLOAT,
        3));
	  
		color_bufferARR[i]->CopyInto (colsARR[i].GetArray(), colsARR[i].GetSize());
	  }
  }

  buffersNeedSetup = false;
}

void csOceanCell::SetupBufferHolder()
{
  if(!bufferHoldersNeedSetup)
    return;
  
  for(uint i = 0; i < 16; i++)
  {
	  csRef<csRenderBufferHolder> bufferHolder;

	  if(bufferHolder == 0)
		  bufferHolder.AttachNew(new csRenderBufferHolder);

	  bufferHolder->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[i]);
	  bufferHolder->SetRenderBuffer(CS_BUFFER_POSITION, vertex_bufferARR[i]);
	  bufferHolder->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_bufferARR[i]);

	  //Ocean color and normals shouldn't change..
	  bufferHolder->SetRenderBuffer(CS_BUFFER_NORMAL, normal_bufferARR[i]);
	  bufferHolder->SetRenderBuffer(CS_BUFFER_COLOR, color_bufferARR[i]);

	  bufferHolderARR.Push(bufferHolder);
  }
  bufferHoldersNeedSetup = false;
}

////////////// csOceanNode //////////////////


csOceanNode::csOceanNode(csVector2 pos, float len, float wid)
{
  gc = pos;
  this->len = len;
  this->wid = wid;
  oHeight = 0;
  
  bbox = csBox3(gc.x, oHeight - 1.0, gc.y, gc.x + len, oHeight + 1.0, gc.y + wid);
}

csOceanNode::~csOceanNode()
{
  
}

csOceanNode csOceanNode::GetLeft() const
{
  return csOceanNode(csVector2(gc.x - len, gc.y), len, wid);
}

csOceanNode csOceanNode::GetRight() const
{
  return csOceanNode(csVector2(gc.x + len, gc.y), len, wid);
}

csOceanNode csOceanNode::GetUp() const
{
  return csOceanNode(csVector2(gc.x, gc.y + wid), len, wid);
}

csOceanNode csOceanNode::GetDown() const
{
  return csOceanNode(csVector2(gc.x, gc.y - wid), len, wid);
}

csVector3 csOceanNode::GetCenter() const
{
  return csVector3(gc.x + (len / 2), oHeight, gc.y + (wid / 2));
}
