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

  if (!index_bufferARR.GetSize())
  {
	 for(uint i = 0; i < 9; i++)
	 {
		  index_bufferARR.Push(  csRenderBuffer::CreateIndexRenderBuffer (
		  (tris.GetSize()*3 + tris_BH.GetSize()*3 + tris_TH.GetSize()*3 + tris_RH.GetSize()*3 + tris_LH.GetSize()*3),
		   CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_INT,
		   0, verts.GetSize()-1) );
	 }
  }
  index_bufferARR[0]->CopyInto (tris.GetArray(), tris.GetSize()*3);
  index_bufferARR[1]->CopyInto (tris_TL.GetArray(), tris_TL.GetSize()*3);
  index_bufferARR[2]->CopyInto (tris_TH.GetArray(), tris_TH.GetSize()*3);
  index_bufferARR[3]->CopyInto (tris_RL.GetArray(), tris_RL.GetSize()*3);
  index_bufferARR[4]->CopyInto (tris_RH.GetArray(), tris_RH.GetSize()*3);
  index_bufferARR[5]->CopyInto (tris_BL.GetArray(), tris_BL.GetSize()*3);
  index_bufferARR[6]->CopyInto (tris_BH.GetArray(), tris_BH.GetSize()*3);
  index_bufferARR[7]->CopyInto (tris_LL.GetArray(), tris_LL.GetSize()*3);
  index_bufferARR[8]->CopyInto (tris_LH.GetArray(), tris_LH.GetSize()*3);
  
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
  
  for(uint i = 0; i < 9; i++)
  {
	  csRef<csRenderBufferHolder> bufferHolder;

	  if(bufferHolder == 0)
		  bufferHolder.AttachNew(new csRenderBufferHolder);

	  bufferHolder->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[i]);
	  bufferHolder->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
	  bufferHolder->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);

	  //Ocean color and normals shouldn't change..
	  bufferHolder->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
	  bufferHolder->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);

	  bufferHolderARR.Push(bufferHolder);
  }

  /*
  if(bufferHolder == 0)
      bufferHolder.AttachNew(new csRenderBufferHolder);
  
    bufferHolder->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[0]);
    bufferHolder->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
    
    //Ocean color and normals shouldn't change..
    bufferHolder->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
   
    if(bufferHolder_TL == 0)
  	  bufferHolder_TL.AttachNew(new csRenderBufferHolder);
  
    bufferHolder_TL->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[1]);
    bufferHolder_TL->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder_TL->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
    //Ocean color and normals shouldn't change..
    bufferHolder_TL->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder_TL->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
  
    if(bufferHolder_TH == 0)
  	  bufferHolder_TH.AttachNew(new csRenderBufferHolder);
  
    bufferHolder_TH->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[2]);
    bufferHolder_TH->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder_TH->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
    //Ocean color and normals shouldn't change..
    bufferHolder_TH->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder_TH->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
  
    if(bufferHolder_RL == 0)
  	  bufferHolder_RL.AttachNew(new csRenderBufferHolder);
  
    bufferHolder_RL->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[3]);
    bufferHolder_RL->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder_RL->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
    //Ocean color and normals shouldn't change..
    bufferHolder_RL->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder_RL->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
  
    if(bufferHolder_RH == 0)
  	  bufferHolder_RH.AttachNew(new csRenderBufferHolder);
  
    bufferHolder_RH->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[4]);
    bufferHolder_RH->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder_RH->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
    //Ocean color and normals shouldn't change..
    bufferHolder_RH->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder_RH->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
  
    if(bufferHolder_BL == 0)
  	  bufferHolder_BL.AttachNew(new csRenderBufferHolder);
  
    bufferHolder_BL->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[5]);
    bufferHolder_BL->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder_BL->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
    //Ocean color and normals shouldn't change..
    bufferHolder_BL->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder_BL->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
  
    if(bufferHolder_BH == 0)
  	  bufferHolder_BH.AttachNew(new csRenderBufferHolder);
  
    bufferHolder_BH->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[6]);
    bufferHolder_BH->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder_BH->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
    //Ocean color and normals shouldn't change..
    bufferHolder_BH->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder_BH->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
  
    if(bufferHolder_LL == 0)
  	  bufferHolder_LL.AttachNew(new csRenderBufferHolder);
  
    bufferHolder_LL->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[7]);
    bufferHolder_LL->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder_LL->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
    //Ocean color and normals shouldn't change..
    bufferHolder_LL->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder_LL->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);
  
    if(bufferHolder_LH == 0)
  	  bufferHolder_LH.AttachNew(new csRenderBufferHolder);
  
    bufferHolder_LH->SetRenderBuffer(CS_BUFFER_INDEX, index_bufferARR[8]);
    bufferHolder_LH->SetRenderBuffer(CS_BUFFER_POSITION, vertex_buffer);
    bufferHolder_LH->SetRenderBuffer(CS_BUFFER_TEXCOORD0, texel_buffer);
  
    //Ocean color and normals shouldn't change..
    bufferHolder_LH->SetRenderBuffer(CS_BUFFER_NORMAL, normal_buffer);
    bufferHolder_LH->SetRenderBuffer(CS_BUFFER_COLOR, color_buffer);*/
  
	
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
