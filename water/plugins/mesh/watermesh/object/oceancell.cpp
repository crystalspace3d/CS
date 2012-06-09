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

void csOceanCell::BoundaryGen()
{
	//csPrintf("Generating boundaries now");

	csDirtyAccessArray<csTriangle> trisTemp;
	csDirtyAccessArray<csTriangle> trisTemp1;
	
	csTriangle TempTri;

	
	trisTemp1 = tris;   // Put all the tris

	while(!trisTemp1.IsEmpty())
	{
		TempTri = trisTemp1.Pop();
		trisTemp.Push(TempTri);
	}
	
	trisTemp1 = tris_RH;  // adds right Higher boundary mesh 

	while(!trisTemp1.IsEmpty())   
	{
		TempTri = trisTemp1.Pop();
		trisTemp.Push(TempTri);
	}

	trisTemp1 = tris_LH;  // adds left Higher boundary mesh 

	while(!trisTemp1.IsEmpty())   
	{
		TempTri = trisTemp1.Pop();
		trisTemp.Push(TempTri);
	}

	trisTemp1 = tris_TL;  // adds top Lower boundary mesh 

	while(!trisTemp1.IsEmpty())   
	{
		TempTri = trisTemp1.Pop();
		trisTemp.Push(TempTri);
	}

	trisTemp1 = tris_BL;  // adds Bottom Lower boundary mesh 

	while(!trisTemp1.IsEmpty())   
	{
		TempTri = trisTemp1.Pop();
		trisTemp.Push(TempTri);
	}
	
	index_buffer->CopyInto (trisTemp.GetArray(), trisTemp.GetSize()*3);
	
}

void csOceanCell::SetupVertices()
{
  if(!isSetup)
  {
    float gran;
    switch(type)
    {
      default:
      case LOD_LEVEL_1:
        gran = 0.2f;
        break;
      case LOD_LEVEL_2:
        gran = 0.4f;
        break;
      case LOD_LEVEL_3:
        gran = 0.8f;
        break;
      case LOD_LEVEL_4:
        gran = 1.6f;
        break;
      case LOD_LEVEL_5:
        gran = 0.4f;
        break;
    }

    uint maxjd = (uint) (len * gran);
    uint maxj = maxjd + 1;
    uint maxid = (uint) (wid * gran);
    uint maxi = maxid + 1;

    for(uint j = 0; j < maxj  ; ++j)
    {
      for(uint i = 0; i < maxi ; ++i)
      {
        verts.Push(csVector3 ((i * wid / maxid), oHeight, (j * len / maxjd)));
        norms.Push(csVector3 (0, 1, 0));
        cols.Push(csColor (0.17f,0.27f,0.26f));
        texs.Push(csVector2((i * wid / maxid) / 1.5, (j * len / maxjd) / 1.5));
      }
    }


    for(uint j = 1; j <  maxjd - 1; ++j)
    {
      for(uint i = 1; i < maxid - 1; ++i)
      {
		  tris.Push(csTriangle ((int)(j * maxi + i), 
			  (int)((j + 1) * maxi + i), 
			  (int)(j * maxi + i + 1)));
		  tris.Push(csTriangle ((int)(j * maxi + i + 1),
			  (int)((j + 1) * maxi + i),
			  (int)((j + 1) * maxi + i + 1)));	
	  }
	}


	
/*
 *   Generation of boundaries tris
 *
 */

	// Generating Top boundaries tris
	
	for(uint j=maxid-1, i=1 ; i < maxid ; i++ )
		{
		    // Low boundaries
			if(i%2)
			{
				tris_TL.Push(csTriangle ((int)(j * maxi + i),
					(int)((j + 1) * maxi + i - 1),
					(int)((j + 1) * maxi + i + 1)));
	
				if (i!=maxid-1)
				{
					tris_TL.Push(csTriangle ((int)(j * maxi + i ), 
							(int)((j + 1) * maxi + i + 1), 
							(int)(j * maxi + i + 1)));
				}
			}
			else 
			{
				tris_TL.Push(csTriangle ((int)(j * maxi + i), 
					(int)((j + 1) * maxi + i), 
					(int)(j * maxi + i + 1)));
			}

			// High boundaries
			if(i%2)
			{
				tris_TH.Push(csTriangle ((int)(j * maxi + i),
					(int)((j + 1) * maxi + i - 1),
					(int)((j + 1) * maxi + i )));
				tris_TH.Push(csTriangle ((int)(j * maxi + i),
					(int)((j + 1) * maxi + i),
					(int)((j + 1) * maxi + i + 1)));
				if (i!=maxid-1)
				{
					tris_TH.Push(csTriangle ((int)(j * maxi + i ), 
							(int)((j + 1) * maxi + i + 1), 
							(int)(j * maxi + i + 1)));
				}
			}
			else 
			{
				tris_TH.Push(csTriangle ((int)(j * maxi + i), 
					(int)((j + 1) * maxi + i), 
					(int)(j * maxi + i + 1)));
			}
		} 
	

	// Generate Bottom boundaries tris
	for(uint j=0, i=1 ; i < maxid ; i++ )
		{
			// Low boundaries
			if(i%2)
			{
				tris_BL.Push(csTriangle ((int)(j * maxi + i - 1),
					(int)((j + 1) * maxi + i),
					(int)(j * maxi + i + 1)));
				if (i!=maxid-1)
				{
					tris_BL.Push(csTriangle ((int)(j * maxi + i + 1), 
						(int)((j + 1) * maxi + i ), 
						(int)((j + 1) * maxi + i + 1)));
				}
			}
			else
			{
				tris_BL.Push(csTriangle ((int)(j * maxi + i), 
					(int)((j + 1) * maxi + i), 
					(int)((j + 1) * maxi + i + 1)));  
			}

			// High boundaries
			if(i%2)
			{
				tris_BH.Push(csTriangle ((int)(j * maxi + i - 1),
					(int)((j + 1) * maxi + i),
					(int)(j * maxi + i )));

				tris_BH.Push(csTriangle ((int)(j * maxi + i ),
					(int)((j + 1) * maxi + i),
					(int)(j * maxi + i + 1)));
				if (i!=maxid-1)
				{
					tris_BH.Push(csTriangle ((int)(j * maxi + i + 1), 
						(int)((j + 1) * maxi + i ), 
						(int)((j + 1) * maxi + i + 1)));
				}
			}
			else
			{
				tris_BH.Push(csTriangle ((int)(j * maxi + i), 
					(int)((j + 1) * maxi + i), 
					(int)((j + 1) * maxi + i + 1)));  
			}
		}
	

	// Generate Left boundaries tris
	
	for(uint j=1, i=0 ; j < maxjd ; j++ )
		{
		// Low boundaries
			if (j%2)
			{
				tris_LL.Push(csTriangle ((int)( (j-1) * maxi + i ), 
					(int)((j + 1) * maxi + i), 
					(int)(j * maxi + i + 1)));	
				if (j!=maxjd-1)
				{
					tris_LL.Push(csTriangle ((int)(j * maxi + i + 1),
						(int)((j + 1) * maxi + i),
						(int)((j + 1) * maxi + i + 1)));
				}
			}
			else
			{
				tris_LL.Push(csTriangle ((int)(j * maxi + i),
					(int)((j + 1) * maxi + i + 1),
					(int)(j * maxi + i + 1)));
			}

			// High boundaries
			if (j%2)
			{
				tris_LH.Push(csTriangle ((int)( (j-1) * maxi + i ), 
					(int)(j * maxi + i), 
					(int)(j * maxi + i + 1)));	
				tris_LH.Push(csTriangle ((int)(j * maxi + i ), 
					(int)((j+1) * maxi + i ), 
					(int)( j * maxi + i +1 )));	
				if (j!=maxjd-1)
				{
					tris_LH.Push(csTriangle ((int)(j * maxi + i + 1),
						(int)((j + 1) * maxi + i),
						(int)((j + 1) * maxi + i + 1)));
				}
			}
			else
			{
				tris_LH.Push(csTriangle ((int)(j * maxi + i),
					(int)((j + 1) * maxi + i + 1),
					(int)(j * maxi + i + 1)));
			}
		}
	

	// Generate Right boundaries tris
	for(uint j=1, i=maxid -1  ; j < maxjd ; j++ )
	{
		// Low boundaries
		if (j%2)
		{
			tris_RL.Push(csTriangle ((int)(j * maxi + i), 
				(int)((j + 1) * maxi + i + 1), 
				(int)((j - 1) * maxi + i + 1)));
			if (j!=maxjd-1)
			{
				tris_RL.Push(csTriangle ((int)(j * maxi + i), 
					(int)((j + 1) * maxi + i), 
					(int)((j + 1) * maxi + i + 1)));
			}
		}
		else
		{
			tris_RL.Push(csTriangle ((int)(j * maxi + i), 
				(int)((j + 1) * maxi + i), 
				(int)(j * maxi + i + 1)));
		}

		// High boundaries
		if (j%2)
		{
			tris_RH.Push(csTriangle ((int)(j * maxi + i), 
				(int)(j * maxi + i + 1), 
				(int)((j - 1) * maxi + i + 1)));
			tris_RH.Push(csTriangle ((int)(j * maxi + i), 
				(int)((j + 1) * maxi + i + 1), 
				(int)(j * maxi + i + 1)));
			if (j!=maxjd-1)
			{
				tris_RH.Push(csTriangle ((int)(j * maxi + i), 
					(int)((j + 1) * maxi + i), 
					(int)((j + 1) * maxi + i + 1)));
			}
		}
		else
		{
			tris_RH.Push(csTriangle ((int)(j * maxi + i), 
				(int)((j + 1) * maxi + i), 
				(int)(j * maxi + i + 1)));
		}
	}

   
    buffersNeedSetup = true;
    isSetup = true;
  }
}

void csOceanCell::SetupBuffers()
{
  if(!buffersNeedSetup)
    return;
  
  if (!vertex_buffer)
  {
    // Create a buffer that doesn't copy the data.
    vertex_buffer = csRenderBuffer::CreateRenderBuffer (
      verts.GetSize(), CS_BUF_STATIC, CS_BUFCOMP_FLOAT,
      3);
  }
  vertex_buffer->CopyInto (verts.GetArray(), verts.GetSize());

  if (!texel_buffer)
  {
    // Create a buffer that doesn't copy the data.
    texel_buffer = csRenderBuffer::CreateRenderBuffer (
      verts.GetSize(), CS_BUF_STATIC, CS_BUFCOMP_FLOAT,
      2);
  }
  texel_buffer->CopyInto (texs.GetArray(), verts.GetSize());

  if (!index_buffer)
  {
      index_buffer = csRenderBuffer::CreateIndexRenderBuffer (
        (tris.GetSize()*3 + tris_BH.GetSize()*3 + tris_TH.GetSize()*3 + tris_RH.GetSize()*3 + tris_LH.GetSize()*3),
        CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_INT,
        0, verts.GetSize()-1);
  }
  index_buffer->CopyInto (tris.GetArray(), tris.GetSize()*3);
  
  if (!normal_buffer)
  {            
    // Create a buffer that doesn't copy the data.
      normal_buffer = csRenderBuffer::CreateRenderBuffer (
        norms.GetSize(), CS_BUF_STATIC, CS_BUFCOMP_FLOAT,
        3);
  }
  normal_buffer->CopyInto (norms.GetArray(), norms.GetSize());

  if (!color_buffer)
  {            
    // Create a buffer that doesn't copy the data.
      color_buffer = csRenderBuffer::CreateRenderBuffer (
        cols.GetSize(), CS_BUF_STATIC, CS_BUFCOMP_FLOAT,
        3);
  }
  color_buffer->CopyInto (cols.GetArray(), cols.GetSize());

  buffersNeedSetup = false;
}

void csOceanCell::SetupBufferHolder()
{
  if(!bufferHoldersNeedSetup)
    return;
  
  if(bufferHolder == 0)
    bufferHolder.AttachNew(new csRenderBufferHolder);

  bufferHolder->SetRenderBuffer(CS_BUFFER_INDEX, index_buffer);
  bufferHolder->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
  bufferHolder->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
  //Ocean color and normals shouldn't change..
  bufferHolder->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
  bufferHolder->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
  
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
  return csOceanNode(csVector2(gc.x + len, gc.y), len, wid);
}

csOceanNode csOceanNode::GetRight() const
{
  return csOceanNode(csVector2(gc.x - len, gc.y), len, wid);
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
