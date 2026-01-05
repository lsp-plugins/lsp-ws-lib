/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 21 янв. 2025 г.
 *
 * lsp-ws-lib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-ws-lib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-ws-lib. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_GL_GRADIENT_H_
#define PRIVATE_GL_GRADIENT_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/types.h>

#include <lsp-plug.in/ws/IGradient.h>

#include <private/gl/Actions.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class LSP_HIDDEN_MODIFIER Gradient: public IGradient
            {
                protected:
                    union
                    {
                        gl::base_gradient_t     sBase;
                        gl::linear_gradient_t   sLinear;
                        gl::radial_gradient_t   sRadial;
                    };
                    bool                bLinear;

                public:
                    explicit Gradient(const gl::linear_gradient_t & params);
                    explicit Gradient(const gl::radial_gradient_t & params);
                    Gradient(const Gradient &) = delete;
                    Gradient(Gradient &&) = delete;
                    virtual ~Gradient() override;

                    Gradient & operator = (const Gradient &) = delete;
                    Gradient & operator = (Gradient &&) = delete;

                public:
                    virtual void set_start(float r, float g, float b, float a) override;
                    virtual void set_stop(float r, float g, float b, float a) override;

                public:
                    inline const gl::linear_gradient_t & linear_gradient() const { return sLinear;  }
                    inline const gl::radial_gradient_t & radial_gradient() const { return sRadial;  }

                public:
                    /**
                     * Return size of gradient in bytes
                     * @return size of gradient in bytes, multiple to 4 floating-point values
                     */
                    size_t  serial_size() const;

                    /**
                     * Serialize gradient contents to buffer
                     * @param buf buffer to serialize gradient contents
                     */
                    float  *serialize(float *buf) const;

                    /**
                     * Check if gradient is linear
                     * @return true if gradient is linear
                     */
                    inline bool is_linear() const { return bLinear; }
            };
        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_GRADIENT_H_ */
