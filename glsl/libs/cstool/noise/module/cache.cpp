// cache.cpp
//
// Copyright (C) 2003, 2004 Jason Bevins
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License (COPYING.txt) for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// The developer's email is jlbezigvins@gmzigail.com (for great email, take
// off every 'zig'.)
//

#include "cstool/noise/module/cache.h"

using namespace CS::Math::Noise::Module;

Cache::Cache ():
  Module (GetSourceModuleCount ()),
  m_isCached (false)
{
}

double Cache::GetValue (double x, double y, double z) const
{
  assert (m_pSourceModule[0] != NULL);

  if (!(m_isCached && x == m_xCache && y == m_yCache && z == m_zCache)) {
    m_cachedValue = m_pSourceModule[0]->GetValue (x, y, z);
    m_xCache = x;
    m_yCache = y;
    m_zCache = z;
  }
  m_isCached = true;
  return m_cachedValue;
}
