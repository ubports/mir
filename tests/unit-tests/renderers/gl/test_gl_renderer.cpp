/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <functional>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/geometry/rectangle.h>
#include <mir/test/fake_shared.h>
#include <mir/test/doubles/mock_gl_buffer.h>
#include <mir/test/doubles/mock_renderable.h>
#include <mir/test/doubles/mock_buffer_stream.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/test/doubles/mock_gl.h>
#include <mir/test/doubles/mock_egl.h>
#include <src/renderers/gl/renderer.h>
#include <mir/test/doubles/stub_gl_display_buffer.h>
#include <mir/test/doubles/mock_gl_display_buffer.h>

using testing::SetArgPointee;
using testing::InSequence;
using testing::Return;
using testing::ReturnRef;
using testing::Pointee;
using testing::AnyNumber;
using testing::AtLeast;
using testing::_;

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace mg=mir::graphics;
namespace mgl=mir::gl;
namespace mrg = mir::renderer::gl;

namespace
{
const GLint stub_v_shader = 1;
const GLint stub_f_shader = 2;
const GLint stub_program = 1;
const GLint transform_uniform_location = 1;
const GLint alpha_uniform_location = 2;
const GLint position_attr_location = 3;
const GLint texcoord_attr_location = 4;
const GLint screen_to_gl_coords_uniform_location = 5;
const GLint tex_uniform_location = 6;
const GLint display_transform_uniform_location = 7;
const GLint centre_uniform_location = 8;

void SetUpMockProgramData(mtd::MockGL &mock_gl)
{
    /* Uniforms and Attributes */
    ON_CALL(mock_gl, glGetAttribLocation(stub_program, "position"))
        .WillByDefault(Return(position_attr_location));
    ON_CALL(mock_gl, glGetAttribLocation(stub_program, "texcoord"))
        .WillByDefault(Return(texcoord_attr_location));

    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "tex"))
        .WillByDefault(Return(tex_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "centre"))
        .WillByDefault(Return(centre_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "display_transform"))
        .WillByDefault(Return(display_transform_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "transform"))
        .WillByDefault(Return(transform_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "screen_to_gl_coords"))
        .WillByDefault(Return(screen_to_gl_coords_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "alpha"))
        .WillByDefault(Return(alpha_uniform_location));
}

class GLRenderer :
    public testing::Test
{
public:

    GLRenderer()
    {
        //Mock defaults
        ON_CALL(mock_gl, glCreateShader(GL_VERTEX_SHADER))
            .WillByDefault(Return(stub_v_shader));
        ON_CALL(mock_gl, glCreateShader(GL_FRAGMENT_SHADER))
            .WillByDefault(Return(stub_f_shader));
        ON_CALL(mock_gl, glCreateProgram())
            .WillByDefault(Return(stub_program));
        ON_CALL(mock_gl, glGetProgramiv(_,_,_))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetShaderiv(_,_,_))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));

        //A mix of defaults and silencing from here on out
        EXPECT_CALL(mock_gl, glUseProgram(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glActiveTexture(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniformMatrix4fv(_, _, GL_FALSE, _))
            .Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniform1f(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniform2f(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glBindBuffer(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glVertexAttribPointer(_, _, _, _, _, _))
            .Times(AnyNumber());
        EXPECT_CALL(mock_gl, glEnableVertexAttribArray(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glDisableVertexAttribArray(_)).Times(AnyNumber());

        mock_buffer = std::make_shared<mtd::MockGLBuffer>();
        EXPECT_CALL(*mock_buffer, gl_bind_to_texture()).Times(AnyNumber());
        EXPECT_CALL(*mock_buffer, native_buffer_base()).Times(AnyNumber());
        EXPECT_CALL(*mock_buffer, id())
            .WillRepeatedly(Return(mir::graphics::BufferID(789)));
        EXPECT_CALL(*mock_buffer, size())
            .WillRepeatedly(Return(mir::geometry::Size{123, 456}));

        renderable = std::make_shared<testing::NiceMock<mtd::MockRenderable>>();
        EXPECT_CALL(*renderable, id()).WillRepeatedly(Return(&renderable));
        EXPECT_CALL(*renderable, buffer()).WillRepeatedly(Return(mock_buffer));
        EXPECT_CALL(*renderable, shaped()).WillRepeatedly(Return(false));
        EXPECT_CALL(*renderable, alpha()).WillRepeatedly(Return(1.0f));
        EXPECT_CALL(*renderable, transformation()).WillRepeatedly(Return(trans));
        EXPECT_CALL(*renderable, screen_position())
            .WillRepeatedly(Return(mir::geometry::Rectangle{{1,2},{3,4}}));
        EXPECT_CALL(mock_gl, glDisable(_)).Times(AnyNumber());

        renderable_list.push_back(renderable);

        InSequence s;
        SetUpMockProgramData(mock_gl);

        EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
            .WillRepeatedly(Return(screen_to_gl_coords_uniform_location));
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mtd::MockGLBuffer> mock_buffer;
    mtd::StubGLDisplayBuffer display_buffer{{{1, 2}, {3, 4}}};
    testing::NiceMock<mtd::MockGLDisplayBuffer> mock_display_buffer;
    std::shared_ptr<testing::NiceMock<mtd::MockRenderable>> renderable;
    mg::RenderableList renderable_list;
    glm::mat4 trans;
};

}

TEST_F(GLRenderer, disables_blending_for_rgbx_surfaces)
{
    InSequence seq;
    EXPECT_CALL(*renderable, shaped())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND));

    mrg::Renderer renderer(display_buffer);
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, enables_blending_for_rgba_surfaces)
{
    EXPECT_CALL(*renderable, shaped()).WillOnce(Return(true));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND)).Times(0);
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));

    mrg::Renderer renderer(display_buffer);
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, enables_blending_for_rgbx_translucent_surfaces)
{
    EXPECT_CALL(*renderable, alpha()).WillRepeatedly(Return(0.5f));
    EXPECT_CALL(*renderable, shaped()).WillOnce(Return(false));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND)).Times(0);
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));

    mrg::Renderer renderer(display_buffer);
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, uses_premultiplied_src_alpha_for_rgba_surfaces)
{
    EXPECT_CALL(*renderable, shaped()).WillOnce(Return(true));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND)).Times(0);
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));
    EXPECT_CALL(mock_gl, glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
                                             GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

    mrg::Renderer renderer(display_buffer);
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, avoids_src_alpha_for_rgbx_blending)  // LP: #1423462
{
    EXPECT_CALL(*renderable, alpha()).WillRepeatedly(Return(0.5f));
    EXPECT_CALL(*renderable, shaped()).WillOnce(Return(false));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND)).Times(0);
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));
    EXPECT_CALL(mock_gl,
                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_CONSTANT_ALPHA,
                                    GL_ZERO, GL_ONE));

    mrg::Renderer renderer(display_buffer);
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, binds_for_every_primitive_when_tessellate_is_overridden)
{
    //'listening to the tests', it would be a bit easier to use a tessellator mock of some sort
    struct OverriddenTessellateRenderer : public mrg::Renderer
    {
        OverriddenTessellateRenderer(
            mg::DisplayBuffer& display_buffer, unsigned int num_primitives) :
            Renderer(display_buffer),
            num_primitives(num_primitives)
        {
        }

        void tessellate(std::vector<mgl::Primitive>& primitives,
                        mg::Renderable const&) const override
        {
            primitives.resize(num_primitives);
            for(GLuint i=0; i < num_primitives; i++)
            {
                auto& p = primitives[i];
                p.type = 0;
                p.tex_id = i % 2;
                p.nvertices = 0;
            }
        }
        unsigned int num_primitives; 
    };

    int bind_count = 6;
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, _))
        .Times(AtLeast(bind_count));

    OverriddenTessellateRenderer renderer(display_buffer, bind_count);
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, clears_all_channels_zero)
{
    InSequence seq;
    EXPECT_CALL(mock_gl, glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_CALL(mock_gl, glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    EXPECT_CALL(mock_gl, glClear(_));

    mrg::Renderer renderer(display_buffer);

    renderer.render(renderable_list);
}

TEST_F(GLRenderer, makes_display_buffer_current_when_created)
{
    EXPECT_CALL(mock_display_buffer, make_current());

    mrg::Renderer renderer(mock_display_buffer);
}

TEST_F(GLRenderer, releases_display_buffer_current_when_destroyed)
{
    mrg::Renderer renderer(mock_display_buffer);

    EXPECT_CALL(mock_display_buffer, release_current());
}

TEST_F(GLRenderer, makes_display_buffer_current_before_rendering)
{
    mrg::Renderer renderer(mock_display_buffer);

    InSequence seq;
    EXPECT_CALL(mock_display_buffer, make_current());
    EXPECT_CALL(mock_gl, glClear(_));

    renderer.render(renderable_list);
}

TEST_F(GLRenderer, swaps_buffers_after_rendering)
{
    mrg::Renderer renderer(mock_display_buffer);

    InSequence seq;
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(mock_display_buffer, swap_buffers());

    renderer.render(renderable_list);
}
