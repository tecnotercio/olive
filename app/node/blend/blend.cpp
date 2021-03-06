/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "blend.h"

BlendNode::BlendNode()
{
  base_input_ = new NodeInput("base_in");
  base_input_->set_data_type(NodeParam::kTexture);
  AddInput(base_input_);

  blend_input_ = new NodeInput("blend_in");
  blend_input_->set_data_type(NodeParam::kTexture);
  AddInput(blend_input_);
}

QString BlendNode::Category() const
{
  return tr("Blend");
}

NodeInput *BlendNode::base_input() const
{
  return base_input_;
}

NodeInput *BlendNode::blend_input() const
{
  return blend_input_;
}

void BlendNode::Retranslate()
{
  base_input_->set_name(tr("Base"));
  blend_input_->set_name(tr("Blend"));
}
